#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifndef DTL_API
#define DTL_API
#endif

#ifdef _WIN32
    #define DTL_CHAR wchar_t
    #define DTL_TEXT(__IN_TEXT__) L##__IN_TEXT__
#else
    #define DTL_CHAR char16_t
    #define DTL_TEXT(__IN_TEXT__) u##__IN_TEXT__
#endif

using dtl_string = std::basic_string<DTL_CHAR>;

class IDynamicTypeLayout;

/** Aligns the pointer or an integer to the given alignment */
template <typename T>
constexpr T Align(T Val, const uint64_t Alignment)
{
    static_assert(std::is_integral_v<T> || std::is_pointer_v<T>, "Align expects an integer or pointer type as Val");
    return static_cast<T>((static_cast<uint64_t>(Val) + Alignment - 1) & ~(Alignment - 1));
}

/** Base class for dynamic types */
class DTL_API FDynamicTypeBase
{
public:
    using ThisClass = FDynamicTypeBase;
    static constexpr bool IsDynamicType = true;

    /// Dynamic types cannot be directly constructed, copied or moved
    FDynamicTypeBase() = delete;
    FDynamicTypeBase( const FDynamicTypeBase& ) = delete;
    FDynamicTypeBase( FDynamicTypeBase&& ) = delete;

    static IDynamicTypeLayout* StaticType();
    static const DTL_CHAR* StaticTypeName() { return DTL_TEXT("FDynamicTypeBase"); }

    static IDynamicTypeLayout* StaticParentType() { return nullptr; }
    static const DTL_CHAR* StaticParentTypeName() { return nullptr; }
};

/**
 * Member Type Descriptors act as a high-level generic wrapper around a type of the field in a dynamic type
 * They allow manipulating the value of the type without knowing it in compile time, which can be useful for implementing custom dynamic type layouts
 * Since this functionality is runtime-based, prefer using compile-type alternatives whenever possible instead
 */
class DTL_API IMemberTypeDescriptor {
public:
    /** Virtual destructor */
    virtual ~IMemberTypeDescriptor() = default;

    /** Returns the name of the type */
    [[nodiscard]] virtual dtl_string GetTypeName() const = 0;
    /** Returns the dynamic type represented by this descriptor, or nullptr if this is not a dynamic type */
    [[nodiscard]] virtual IDynamicTypeLayout* GetDynamicType() const { return nullptr; }

    /** Returns the size of the member */
    [[nodiscard]] virtual size_t GetMemberSize() const = 0;
    /** Returns the minimum alignment required for this member */
    [[nodiscard]] virtual size_t GetMemberAlignment() const = 0;

    /** Initializes the value of this member */
    virtual void EmplaceValue(void* PlacementStorage) const = 0;
    /** Destroys the value of this member */
    virtual void DestructValue(void* Data) const = 0;
    /** Copies the value from one place to another */
    virtual void CopyAssignValue(void* Dest, const void* Src) const = 0;
};

/** Base class for dynamic type members. Can be inherited to allow additional information to member declarations, which is useful in some rare circumstances */
class DTL_API FDynamicTypeMember {
protected:
    dtl_string MemberName;
    IMemberTypeDescriptor* MemberType;
    bool bIsOptionalMember{false};
    int64_t MemberOffset{-1};
public:
    FDynamicTypeMember(const dtl_string& InMemberName, IMemberTypeDescriptor* InMemberType, bool bInIsOptional = false) : MemberName(InMemberName), MemberType(InMemberType), bIsOptionalMember(bInIsOptional) {}
    virtual ~FDynamicTypeMember() = default;

    [[nodiscard]] const dtl_string& GetName() const { return MemberName; }
    [[nodiscard]] IMemberTypeDescriptor* GetType() const { return MemberType; }
    [[nodiscard]] int64_t GetMemberOffset() const { return MemberOffset; }
    [[nodiscard]] bool IsOptionalMember() const { return bIsOptionalMember; }

    /** Converts a pointer to the base of the dynamic type to the pointer to this instance member */
    template<typename T>
    T* ContainerPtrToValuePtr(void* ContainerPtr, const int32_t ArrayIndex = 0) const
    {
        // If this member is unresolved, return nullptr
        if (GetMemberOffset() < 0)
        {
            return nullptr;
        }
        return static_cast<T*>(static_cast<uint8_t*>(ContainerPtr) + GetMemberOffset() + ArrayIndex * GetType()->GetMemberSize());
    }

    /** Converts a pointer to the base of the dynamic type to the pointer to this instance member */
    template<typename T>
    const T* ContainerPtrToValuePtr(const void* ContainerPtr, const int32_t ArrayIndex = 0) const
    {
        // If this member is unresolved, return nullptr
        if (GetMemberOffset() < 0)
        {
            return nullptr;
        }
        return static_cast<const T*>(static_cast<const uint8_t*>(ContainerPtr) + GetMemberOffset() + ArrayIndex * GetType()->GetMemberSize());
    }

    /** Updates member offset directly. Only to be called by InitializeDynamicType! */
    void Internal_SetupMemberOffset(const int64_t InMemberOffset) { MemberOffset = InMemberOffset; }
};

/// A function pointer to a generic type-less function
typedef void (*GenericFunctionPtr);

class DTL_API FDynamicTypeVirtualFunction
{
protected:
    dtl_string FunctionName;
    int64_t VirtualFunctionTableDisplacement{-1};
    int64_t VirtualFunctionTableOffset{-1};
    bool bIsOptional{false};
public:
    FDynamicTypeVirtualFunction(const dtl_string& InFunctionName, bool bInIsOptional) : FunctionName(InFunctionName), bIsOptional(bInIsOptional) {}
    virtual ~FDynamicTypeVirtualFunction() = default;

    [[nodiscard]] const dtl_string& GetName() const { return FunctionName; }
    [[nodiscard]] int64_t GetVirtualFunctionTableDisplacement() const { return VirtualFunctionTableDisplacement; }
    [[nodiscard]] int64_t GetVirtualFunctionTableOffset() const { return VirtualFunctionTableOffset; }
    [[nodiscard]] bool IsOptionalVirtualFunction() const { return bIsOptional; }

    /** Converts a pointer to the base of the dynamic type to the function pointer to this virtual function implementation for the type */
    GenericFunctionPtr ContainerPtrToVirtualFunctionPtr(const void* ContainerPtr) const
    {
        // If this function is unresolved, return nullptr
        if ( GetVirtualFunctionTableDisplacement() < 0 || GetVirtualFunctionTableOffset() < 0 )
        {
            return nullptr;
        }
        // Retrieve virtual function table address
        const GenericFunctionPtr* VirtualFunctionTable = *reinterpret_cast<const GenericFunctionPtr* const*>(static_cast<const uint8_t*>(ContainerPtr) + GetVirtualFunctionTableDisplacement());
        // Retrieve function at the offset in the virtual function table
        return VirtualFunctionTable[GetVirtualFunctionTableOffset() / sizeof(GenericFunctionPtr)];
    }

    /** Updates virtual function offset and displacement directly. Only to be called by InitializeDynamicType! */
    void Internal_SetupFunctionOffsetAndDisplacement(const int64_t InVirtualFunctionTableDisplacement, const int64_t InVirtualFunctionTableOffset)
    {
        VirtualFunctionTableDisplacement = InVirtualFunctionTableDisplacement;
        VirtualFunctionTableOffset = InVirtualFunctionTableOffset;
    }
};

/**
 * Dynamic Type Layout calculates the locations of the members of the type, virtual functions, and provides functions
 * to allow performing common operations on the dynamic types, such as initialization, destruction, and copying
 * Types might have unique layouts based on the requirements that the type has.
 */
class DTL_API IDynamicTypeLayout {
protected:
    dtl_string TypeName;
    std::vector<FDynamicTypeMember*> TypeMembers;
    std::vector<FDynamicTypeVirtualFunction*> VirtualFunctions;
    IDynamicTypeLayout* ParentType{};
public:
    IDynamicTypeLayout(const dtl_string& InTypeName, IDynamicTypeLayout* InParentType, const std::vector<FDynamicTypeMember*>& InTypeMembers, const std::vector<FDynamicTypeVirtualFunction*>& InVirtualFunctions);
    virtual ~IDynamicTypeLayout() = default;

    // Dynamic types cannot be copied or moved
    IDynamicTypeLayout(const IDynamicTypeLayout&) = delete;
    IDynamicTypeLayout(IDynamicTypeLayout&&) = delete;

    [[nodiscard]] const dtl_string& GetTypeName() const { return TypeName; }
    [[nodiscard]] const std::vector<FDynamicTypeMember*>& GetTypeMembers() const { return TypeMembers; }
    [[nodiscard]] IDynamicTypeLayout* GetParentType() const { return ParentType; }

    /** Note that this function will NOT check the parent type */
    [[nodiscard]] FDynamicTypeMember* FindTypeMember(const dtl_string& MemberName) const;
    /** Finds the virtual function in this type by name. Note that this function will also not check the parent type */
    [[nodiscard]] FDynamicTypeVirtualFunction* FindVirtualFunction(const dtl_string& VirtualFunctionName) const;

    /** Returns the ID token for this type. This is used for casting the type implementation */
    [[nodiscard]] virtual uintptr_t GetTypeIdToken() const = 0;
    /** Returns true if this type has the same type ID as the passed token or is a child of a type having that token */
    [[nodiscard]] virtual bool IsSameOrChildOfTypeId(const uintptr_t TypeIdToken) const { return TypeIdToken == GetTypeIdToken(); }

    /** Called once when this type is constructed to initialize it with data */
    virtual void InitializeDynamicType() {}
    /** Initializes the instance of the type at the provided memory location */
    virtual void EmplaceTypeInstance(void* PlacementStorage) const = 0;
    /** Destroys the instance of the type at the provided memory location */
    virtual void DestructTypeInstance(void* TypeInstance) const = 0;
    /** Copies the data from one type instance to another. Note that this function is modeled after the copy assignment operator, so DestInstance must be a valid type instance, and not a placement storage */
    virtual void CopyAssignTypeInstance(void* DestInstance, const void* SrcInstance) const = 0;

    /** @return the current size of the type, or -1 if not computed yet */
    [[nodiscard]] virtual size_t GetSize() const = 0;
    /** @return the current size of the type, or -1 if not computed yet */
    [[nodiscard]] virtual size_t GetMinAlignment() const = 0;
};

/** Attempts to cast a dynamic type implementation to the provided class */
template<typename InDynamicTypeImplCastType>
InDynamicTypeImplCastType* CastDynamicTypeImpl(IDynamicTypeLayout* InDynamicTypeImpl)
{
    if (InDynamicTypeImpl != nullptr && InDynamicTypeImpl->IsSameOrChildOfTypeId(InDynamicTypeImplCastType::StaticTypeIdToken()))
    {
        return static_cast<InDynamicTypeImplCastType*>(InDynamicTypeImpl);
    }
    return nullptr;
}

using CollectTypeMembersFunc = void(*)(std::vector<FDynamicTypeMember*>&, std::vector<FDynamicTypeVirtualFunction*>&);

template<typename TypeImplClass, typename... ExtraArgTypes>
std::unique_ptr<TypeImplClass> ConstructPrivateStaticType(TypeImplClass const dtl_string& InTypeName, IDynamicTypeLayout* InParentType, CollectTypeMembersFunc InCollectTypeMembers, ExtraArgTypes... Args)
{
    std::vector<FDynamicTypeMember*> CollectedMembers;
    std::vector<FDynamicTypeVirtualFunction*> CollectedVirtualFunctions;
    InCollectTypeMembers(CollectedMembers, CollectedVirtualFunctions);
    std::unique_ptr<TypeImplClass> NewTypeInstance = std::make_unique<TypeImplClass>(InTypeName, InParentType, CollectedMembers, CollectedVirtualFunctions, std::forward<ExtraArgTypes>(Args...));
    NewTypeInstance->InitializeDynamicType();
    return std::move(NewTypeInstance);
}
