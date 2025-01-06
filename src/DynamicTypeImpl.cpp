#include "DynamicTypeImpl.h"
#include <stdexcept>

/** Empty type is a type with no members */
class DTL_API EmptyDynamicType : public IDynamicTypeLayout
{
public:
    EmptyDynamicType(const dtl_string& InTypeName, IDynamicTypeLayout* InParentType, const std::vector<FDynamicTypeMember*>& InTypeMembers, const std::vector<FDynamicTypeVirtualFunction*>& InVirtualFunctions) : IDynamicTypeLayout(InTypeName, InParentType, InTypeMembers, InVirtualFunctions) {}

    static uintptr_t StaticTypeIdToken();
    [[nodiscard]] uintptr_t GetTypeIdToken() const override { return StaticTypeIdToken(); }
    void EmplaceTypeInstance(void* Instance, int32_t InternalFlags) const override {}
    void DestructTypeInstance(void* Instance, int32_t InternalFlags) const override {}
    void CopyAssignTypeInstance(void* DestInstance, const void* SrcInstance, int32_t InternalFlags) const override {}
    [[nodiscard]] int32_t GetSize() const override { return 0; }
    [[nodiscard]] int32_t GetMinAlignment() const override { return 1; }
};

uintptr_t EmptyDynamicType::StaticTypeIdToken()
{
    static uint8_t StaticTypeIdToken;
    return reinterpret_cast<uintptr_t>(&StaticTypeIdToken);
}

IDynamicTypeLayout* FDynamicTypeBase::StaticType()
{
    static EmptyDynamicType EmptyDynamicType(StaticTypeName(), nullptr, {}, {});
    return &EmptyDynamicType;
}

IDynamicTypeLayout::IDynamicTypeLayout(const dtl_string& InTypeName, IDynamicTypeLayout* InParentType, const std::vector<FDynamicTypeMember*>& InTypeMembers, const std::vector<FDynamicTypeVirtualFunction*>& InVirtualFunctions)
    : TypeName(InTypeName), TypeMembers(InTypeMembers), VirtualFunctions(InVirtualFunctions), ParentType(InParentType)
{
}

FDynamicTypeMember* IDynamicTypeLayout::FindTypeMember(const dtl_string& MemberName) const
{
    for (FDynamicTypeMember* Member : TypeMembers)
    {
        if (Member->GetName() == MemberName)
        {
            return Member;
        }
    }
    return nullptr;
}

FDynamicTypeVirtualFunction* IDynamicTypeLayout::FindVirtualFunction(const dtl_string& VirtualFunctionName) const
{
    for (FDynamicTypeVirtualFunction* VirtualFunction : VirtualFunctions)
    {
        if (VirtualFunction->GetName() == VirtualFunctionName)
        {
            return VirtualFunction;
        }
    }
    return nullptr;
}

AutoTypeLayout::AutoTypeLayout(const dtl_string& InTypeName, IDynamicTypeLayout* InParentType,
    const std::vector<FDynamicTypeMember*>& InTypeMembers, const std::vector<FDynamicTypeVirtualFunction*>& InVirtualFunctions) :
    IDynamicTypeLayout(InTypeName, InParentType, InTypeMembers, InVirtualFunctions)
{
}

uintptr_t AutoTypeLayout::StaticTypeIdToken()
{
}

void AutoTypeLayout::InitializeDynamicType()
{
    size_t CurrentTypeOffset = ParentType ? ParentType->GetSize() : 0;
    size_t CurrentTypeAlignment = ParentType ? ParentType->GetMinAlignment() : 1;

    // If our parent type is also an auto type layout, retrieve data from it
    if (const AutoTypeLayout* ParentAutoTypeLayout = CastDynamicTypeImpl<AutoTypeLayout>(ParentType))
    {
        // Copy virtual function table displacement and vtable itself from the parent type
        VirtualFunctionTableDisplacement = ParentAutoTypeLayout->VirtualFunctionTableDisplacement;
        VirtualFunctionTable = ParentAutoTypeLayout->VirtualFunctionTable;
    }

    // If we have virtual functions for this type, but do not have a virtual function table yet, allocate one
    if (VirtualFunctionTableDisplacement == -1 && !VirtualFunctions.empty())
    {
        constexpr size_t VirtualFunctionTableAlignment = alignof(const GenericFunctionPtr**);
        constexpr size_t VirtualFunctionTableSize = sizeof(const GenericFunctionPtr**);

        // Align the current offset and assign the displacement from it
        CurrentTypeOffset = Align(CurrentTypeOffset, VirtualFunctionTableAlignment);
        VirtualFunctionTableDisplacement = static_cast<int64_t>(CurrentTypeOffset);

        // Increment the offset and take the alignment requirement of the vtable into the type alignment
        CurrentTypeOffset += VirtualFunctionTableSize;
        CurrentTypeAlignment = std::max(CurrentTypeAlignment, VirtualFunctionTableAlignment);
    }

    // Layout virtual functions in the virtual function table. They should not be in the vtable yet, so just append them to the end
    for (FDynamicTypeVirtualFunction* VirtualFunction : VirtualFunctions)
    {
        const size_t VirtualFunctionTableOffset = sizeof(GenericFunctionPtr) * VirtualFunctionTable.size();
        VirtualFunction->Internal_SetupFunctionOffsetAndDisplacement(VirtualFunctionTableDisplacement, static_cast<int64_t>(VirtualFunctionTableOffset));
        VirtualFunctionTable.push_back(&PureVirtualFunctionCalled);
    }

    // Layout members in memory after the parent class
    for (FDynamicTypeMember* Member : TypeMembers)
    {
        const size_t MemberAlignment = Member->GetType()->GetMemberAlignment();
        const size_t MemberSize = Member->GetType()->GetMemberSize();

        // Make sure the current offset is aligned to the minimum alignment of this member
        CurrentTypeOffset = Align(CurrentTypeOffset, MemberAlignment);
        // Assign the offset to the member
        Member->Internal_SetupMemberOffset(static_cast<int64_t>(CurrentTypeOffset));

        // Increment the offset to account for the member size, and take the maximum alignment between current type alignment and member alignment
        CurrentTypeOffset += MemberSize;
        CurrentTypeAlignment = std::max(CurrentTypeAlignment, MemberAlignment);
    }

    // Assign calculated size and alignment of the structure. Note that type size must always be a multiple of it's alignment
    CurrentTypeOffset = Align(CurrentTypeOffset, CurrentTypeAlignment);
    CalculatedSize = CurrentTypeOffset;
    CalculatedAlignment = CurrentTypeAlignment;
}

void AutoTypeLayout::RegisterVirtualFunctionOverride(const FDynamicTypeVirtualFunction* InVirtualFunction, GenericFunctionPtr NewFunctionPointer)
{
    // Find the virtual function table entry for this function
    if (InVirtualFunction->GetVirtualFunctionTableDisplacement() != VirtualFunctionTableDisplacement)
    {
        throw std::runtime_error("RegisterVirtualFunctionOverride called with invalid virtual function (displacement does not match the class displacement)");
    }
    const int64_t VirtualFunctionIndex = InVirtualFunction->GetVirtualFunctionTableOffset() / static_cast<int64_t>(sizeof(GenericFunctionPtr));
    if (VirtualFunctionIndex < 0 || VirtualFunctionIndex >= VirtualFunctionTable.size())
    {
        throw std::runtime_error("RegisterVirtualFunctionOverride called with invalid virtual function (virtual function offset is invalid)");
    }
    VirtualFunctionTable[VirtualFunctionIndex] = NewFunctionPointer;
}

__declspec(noinline) void AutoTypeLayout::PureVirtualFunctionCalled()
{
    __debugbreak();
    abort();
}

void AutoTypeLayout::EmplaceTypeInstance(void* Instance) const
{
    // Parent type starts at offset 0
    if (ParentType)
    {
        ParentType->EmplaceTypeInstance(Instance);
    }

    // Override the virtual table at it's offset if we have a virtual function table
    if (VirtualFunctionTableDisplacement != -1)
    {
        // This assumes that this type layout will never be moved, which is a reasonable assumption to make
        const auto** VirtualFunctionTablePtr = reinterpret_cast<const GenericFunctionPtr**>(static_cast<uint8_t*>(Instance) + VirtualFunctionTableDisplacement);
        *VirtualFunctionTablePtr = VirtualFunctionTable.data();
    }

    // Our type members follow
    for (const FDynamicTypeMember* Member : TypeMembers)
    {
        Member->GetType()->EmplaceValue(Member->ContainerPtrToValuePtr<void>(Instance));
    }
}

void AutoTypeLayout::DestructTypeInstance(void* Instance) const
{
    // Parent type starts at offset 0
    if (ParentType)
    {
        ParentType->DestructTypeInstance(Instance);
    }

    // Our type members follow
    for (const FDynamicTypeMember* Member : TypeMembers)
    {
        Member->GetType()->DestructValue(Member->ContainerPtrToValuePtr<void>(Instance));
    }
}

void AutoTypeLayout::CopyAssignTypeInstance(void* DestInstance, const void* SrcInstance) const
{
    // Parent type starts at offset 0
    if (ParentType)
    {
        ParentType->CopyAssignTypeInstance(DestInstance, SrcInstance);
    }

    // Our type members follow
    for (const FDynamicTypeMember* Member : TypeMembers)
    {
        Member->GetType()->CopyAssignValue(Member->ContainerPtrToValuePtr<void>(DestInstance), Member->ContainerPtrToValuePtr<void>(SrcInstance));
    }
}
