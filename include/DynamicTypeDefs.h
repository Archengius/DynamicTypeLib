#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#ifndef DTL_API
#define DTL_API
#endif
#define DTL_TEXT(__IN_TEXT__) L##__IN_TEXT__

class IDynamicTypeInterface;

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

    static IDynamicTypeInterface* StaticType();
    static const wchar_t* StaticTypeName() { return DTL_TEXT("FDynamicTypeBase"); }

    static IDynamicTypeInterface* StaticParentType() { return nullptr; }
    static const wchar_t* StaticParentTypeName() { return nullptr; }
};

/** Describes a type of the member in the dynamically typed object */
class DTL_API IMemberTypeDescriptor {
public:
    /** Virtual destructor */
    virtual ~IMemberTypeDescriptor() = default;

    /** Returns the name of the type */
    virtual std::wstring GetTypeName() const = 0;

    /** Returns the dynamic type represented by this descriptor, or nullptr if this is not a dynamic type */
    virtual IDynamicTypeInterface* GetDynamicType() const { return nullptr; }

    /** Returns the size of the member */
    virtual int32_t GetMemberSize() const = 0;

    /** Returns the minimum alignment required for this member */
    virtual int32_t GetMemberAlignment() const = 0;

    /** Initializes the value of this member */
    virtual void InitializeValue( void* Data ) const = 0;

    /** Destroys the value of this member */
    virtual void DestroyValue( void* Data ) const = 0;

    /** Copies the value from one place to another */
    virtual void CopyValue( void* Dest, const void* Src ) const = 0;

    /** Resets the property value to the default */
    virtual void ClearValue( void* Data ) const = 0;
};

/** Base interface for the abstract type member */
class DTL_API IDynamicTypeMember {
public:
    /** Virtual destructor */
    virtual ~IDynamicTypeMember() = default;

    /** Returns the name of the member */
    virtual std::wstring GetName() const = 0;

    /** Returns the type of the member */
    virtual IMemberTypeDescriptor* GetType() const = 0;

    /** Returns the offset of this member in the type */
    virtual int32_t GetMemberOffset() const = 0;

    virtual bool IsOptionalMember() const = 0;

    /** Converts a pointer to the base of the dynamic type to the pointer to this instance member */
    template<typename T>
    T* ContainerPtrToValuePtr( void* ContainerPtr, int32_t ArrayIndex = 0 ) const
    {
        if ( GetMemberOffset() < 0 )
        {
            return nullptr;
        }
        return (T*) ( ((uint8_t*) ContainerPtr) + GetMemberOffset() + ArrayIndex * GetType()->GetMemberSize() );
    }

    /** Converts a pointer to the base of the dynamic type to the pointer to this instance member */
    template<typename T>
    const T* ContainerPtrToValuePtr( const void* ContainerPtr, int32_t ArrayIndex = 0 ) const
    {
        if ( GetMemberOffset() < 0 )
        {
            return nullptr;
        }
        return (const T*) ( ((const uint8_t*) ContainerPtr) + GetMemberOffset() + ArrayIndex * GetType()->GetMemberSize() );
    }

    /** Updates member offset directly. Only to be called by InitializeDynamicType! */
    virtual void Internal_SetupMemberOffset( int32_t InMemberOffset ) = 0;
};

/** Basic implementation of the dynamic type member, does not store any additional information */
class DTL_API FDynamicTypeMember : public IDynamicTypeMember {
protected:
    std::wstring MemberName;
    IMemberTypeDescriptor* MemberType;
    bool bIsOptionalMember{false};
    int32_t MemberOffset{-1};
public:
    FDynamicTypeMember( std::wstring InMemberName, IMemberTypeDescriptor* InMemberType, bool InIsOptional = false ) : MemberName(std::move( InMemberName )), MemberType( InMemberType ), bIsOptionalMember( InIsOptional ) {}

    std::wstring GetName() const override { return MemberName; }
    IMemberTypeDescriptor* GetType() const override { return MemberType; }
    int32_t GetMemberOffset() const override { return MemberOffset; }
    bool IsOptionalMember() const override { return bIsOptionalMember; }
    void Internal_SetupMemberOffset( int32_t InMemberOffset ) override { MemberOffset = InMemberOffset; }
};

/** Describes a dynamic type with a layout dependent on other dynamic types */
class DTL_API IDynamicTypeInterface {
public:
    /** Virtual destructor */
    virtual ~IDynamicTypeInterface() = default;

    /** Registers a type member inside of this type */
    virtual void RegisterTypeMember( IDynamicTypeMember* InTypeMember ) = 0;
    virtual void GetAllTypeMembers( std::vector<IDynamicTypeMember*>& OutTypeMembers ) const = 0;
    virtual IDynamicTypeMember* FindTypeMember( const std::wstring& MemberName ) const = 0;

    /** Initializes the instance of the type at the provided memory location */
    virtual bool InitializeTypeInstance( void* Instance, int32_t InternalFlags = 0 ) const = 0;
    /** Destroys the instance of the type at the provided memory location */
    virtual void DestroyTypeInstance( void* Instance, int32_t InternalFlags = 0 ) const = 0;
    /** Copies the data from one type instance to another */
    virtual void CopyTypeInstance( void* DestInstance, const void* SrcInstance, int32_t InternalFlags = 0 ) const = 0;

    /** Populates the vector with the list of dependencies this type has on other types. These will need to be computed before the layout of that type can be computed */
    virtual void GetLayoutDependencies( std::vector<IDynamicTypeInterface*>& LayoutDependencies ) const {};

    /** @return true if the layout has already been computed, false otherwise */
    virtual bool IsLayoutComputed() const = 0;
    /** Recalculates the layout of this type, additionally with provided options. @return true if the layout type has been successfully recalculated, false otherwise */
    virtual bool RecomputeLayout( bool bForce = false ) = 0;

    /** @return the current size of the type, or -1 if not computed yet */
    virtual int32_t GetSize() const = 0;
    /** @return the current size of the type, or -1 if not computed yet */
    virtual int32_t GetMinAlignment() const = 0;

    /** Accessor that makes sure the type size is aligned */
    int32_t GetAlignedSize() const {
        const int32_t MinAlignment = GetMinAlignment();
        return ( GetSize() + MinAlignment - 1 ) / MinAlignment * MinAlignment;
    }
};

/// Allocates dynamic type on the stack in form of a local variable. the instance will automatically be deleted when the variable goes out of scope.
#define DYNAMIC_TYPE_INSTANCE_ON_STACK( InDynamicType, InVariableName ) TDynamicTypeInstance<InDynamicType> InVariableName( alloca( InDynamicType::StaticType()->GetAlignedSize() ) );

/** Allows producing dynamic type instances */
class DTL_API FDynamicTypeInstance {
protected:
    IDynamicTypeInterface* DynamicType{};
    void* InstanceDataPtr{};
    bool OwnsInstanceMemory{};
public:
    /// This constructor takes a direct reference to the memory
    FDynamicTypeInstance( IDynamicTypeInterface* InDynamicType, void* InInstanceMemory ) : DynamicType( InDynamicType ), InstanceDataPtr( InInstanceMemory ), OwnsInstanceMemory( false )
    {
        if ( DynamicType && InstanceDataPtr )
        {
            DynamicType->InitializeTypeInstance( InstanceDataPtr );
        }
    }

    explicit FDynamicTypeInstance( IDynamicTypeInterface* InDynamicType ) : DynamicType( InDynamicType )
    {
        if ( InDynamicType )
        {
            InstanceDataPtr = malloc( InDynamicType->GetAlignedSize() );
            OwnsInstanceMemory = true;
            InDynamicType->InitializeTypeInstance( InstanceDataPtr );
        }
    }

    /// This constructor copies the memory
    FDynamicTypeInstance( IDynamicTypeInterface* InDynamicType, const void* InInstanceMemorySrc ) : FDynamicTypeInstance( InDynamicType )
    {
        if ( DynamicType && InInstanceMemorySrc )
        {
            DynamicType->CopyTypeInstance( InstanceDataPtr, InInstanceMemorySrc );
        }
    }

    FDynamicTypeInstance( const FDynamicTypeInstance& InTypeInstance ) : DynamicType( InTypeInstance.DynamicType )
    {
        // If the provided instance owns the memory, we need to copy from its data
        if ( InTypeInstance.OwnsInstanceMemory )
        {
            InstanceDataPtr = malloc( DynamicType->GetAlignedSize() );
            OwnsInstanceMemory = true;

            DynamicType->InitializeTypeInstance( InstanceDataPtr );
            DynamicType->CopyTypeInstance( InstanceDataPtr, InTypeInstance.InstanceDataPtr );
        }
        // Otherwise, we just reference the exact same external memory
        else
        {
            InstanceDataPtr = InTypeInstance.InstanceDataPtr;
            OwnsInstanceMemory = false;
        }
    }

    FDynamicTypeInstance( FDynamicTypeInstance&& InTypeInstance )  noexcept : DynamicType( InTypeInstance.DynamicType ), InstanceDataPtr( InTypeInstance.InstanceDataPtr ), OwnsInstanceMemory( InTypeInstance.OwnsInstanceMemory )
    {
        InTypeInstance.InstanceDataPtr = nullptr;
        InTypeInstance.OwnsInstanceMemory = false;
    }

    ~FDynamicTypeInstance()
    {
        if ( InstanceDataPtr && OwnsInstanceMemory )
        {
            if ( DynamicType )
            {
                DynamicType->DestroyTypeInstance( InstanceDataPtr );
            }
            free( InstanceDataPtr );
            InstanceDataPtr = nullptr;
            OwnsInstanceMemory = false;
        }
    }

    IDynamicTypeInterface* GetDynamicType() const { return DynamicType; }
    void* GetInstanceData() const { return InstanceDataPtr; }
};

/** Templated version of FDynamicTypeInstance */
template<typename T>
class TDynamicTypeInstance : public FDynamicTypeInstance
{
public:
    // Expose parent constructors, but limit the input types to be compatible with our type
    explicit TDynamicTypeInstance( void* InInstanceMemory ) : FDynamicTypeInstance( T::StaticType(), InInstanceMemory ) {}
    TDynamicTypeInstance() : FDynamicTypeInstance( T::StaticType() ) {}
    TDynamicTypeInstance( const TDynamicTypeInstance<T>& InTypeInstance ) : FDynamicTypeInstance( static_cast<const FDynamicTypeInstance&>( InTypeInstance ) ) {}
    TDynamicTypeInstance( TDynamicTypeInstance<T>&& InTypeInstance )  noexcept : FDynamicTypeInstance( static_cast<FDynamicTypeInstance&&>( InTypeInstance ) ) {}
    explicit TDynamicTypeInstance( const void* InInstanceMemorySrc ) : FDynamicTypeInstance( T::StaticType(), InInstanceMemorySrc ) {}

    T* Get() const { return static_cast<T*>( InstanceDataPtr ); }
    T& operator*() const { return *Get(); }
    T* operator->() const { return Get(); }
    explicit operator bool() const { return Get() != nullptr; }
};
