#pragma once

#include <xstring>
#include <memory>
#include "DynamicTypeDefs.h"

/** Implementation of the IMemberTypeDescriptor for a statically known type (e.g. a primitive like int32, FString, float, double) */
template<typename T>
class TMemberTypeDescriptor : public IMemberTypeDescriptor {
protected:
    dtl_string TypeNameReference;
public:
    explicit TMemberTypeDescriptor( dtl_string InTypeName ) : TypeNameReference(std::move(InTypeName)) {}

    static const T* GetValuePtr(const void* Data) { return static_cast<const T*>(Data); }
    static T* GetValuePtr(void* Data) { return static_cast<T*>(Data); }

    [[nodiscard]] dtl_string GetTypeName() const override { return TypeNameReference; }
    [[nodiscard]] size_t GetMemberSize() const override { return sizeof(T); }
    [[nodiscard]] size_t GetMemberAlignment() const override { return alignof(T); }
    void EmplaceValue(void* PlacementStorage) const override { new (PlacementStorage) T(); }
    void DestructValue(void* Data) const override { GetValuePtr(Data)->~T(); }
    void CopyAssignValue(void* Dest, const void* Src) const override { *GetValuePtr(Dest) = *GetValuePtr(Src); }

    static TMemberTypeDescriptor* StaticDescriptor(const DTL_CHAR* TypeName)
    {
        static TMemberTypeDescriptor StaticDescriptor{ TypeName };
        return &StaticDescriptor;
    }
};

/** Member type descriptor for the dynamic type */
class FDynamicMemberTypeDescriptor : public IMemberTypeDescriptor
{
protected:
    IDynamicTypeLayout* DynamicType{};
public:
    explicit FDynamicMemberTypeDescriptor(IDynamicTypeLayout* InDynamicType) : DynamicType(InDynamicType) {}

    [[nodiscard]] dtl_string GetTypeName() const override { return DynamicType->GetTypeName(); }
    [[nodiscard]] size_t GetMemberSize() const override { return DynamicType->GetSize(); }
    [[nodiscard]] size_t GetMemberAlignment() const override { return DynamicType->GetMinAlignment(); }
    void EmplaceValue(void* PlacementStorage) const override { DynamicType->EmplaceTypeInstance(PlacementStorage); }
    void DestructValue(void* Data) const override { DynamicType->DestructTypeInstance(Data); }
    void CopyAssignValue(void* Dest, const void* Src) const override { DynamicType->CopyAssignTypeInstance(Dest, Src); }
    [[nodiscard]] IDynamicTypeLayout* GetDynamicType() const override { return DynamicType; }
};

/**
 * Automatic type layout that will lay out members in the order of declaration.
 * Supports virtual table management. If there are virtual functions, they will be bound to this type's vtable.
 * Virtual function implementations can be registered RegisterVirtualFunctionOverride. By default, all virtual functions are pure and calling them will result in a pure handler being called.
 */
class DTL_API AutoTypeLayout : public IDynamicTypeLayout {
protected:
    size_t CalculatedSize{0};
    size_t CalculatedAlignment{1};
    int64_t VirtualFunctionTableDisplacement{-1};
    std::vector<GenericFunctionPtr> VirtualFunctionTable;
public:
    AutoTypeLayout(const dtl_string& InTypeName, IDynamicTypeLayout* InParentType, const std::vector<FDynamicTypeMember*>& InTypeMembers, const std::vector<FDynamicTypeVirtualFunction*>& InVirtualFunctions);

    /** Allows overriding the default implementation of the provided virtual function */
    void RegisterVirtualFunctionOverride(const FDynamicTypeVirtualFunction* InVirtualFunction, GenericFunctionPtr NewFunctionPointer);

    static uintptr_t StaticTypeIdToken();
    [[nodiscard]] uintptr_t GetTypeIdToken() const override { return StaticTypeIdToken(); }
    void InitializeDynamicType() override;
    void EmplaceTypeInstance(void* Instance) const override;
    void DestructTypeInstance(void* Instance) const override;
    void CopyAssignTypeInstance(void* DestInstance, const void* SrcInstance) const override;
    [[nodiscard]] size_t GetSize() const override { return CalculatedSize; }
    [[nodiscard]] size_t GetMinAlignment() const override { return CalculatedAlignment; }
private:
    static void PureVirtualFunctionCalled();
};
