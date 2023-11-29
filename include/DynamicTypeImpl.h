#pragma once

#include <xstring>
#include <memory>
#include "DynamicTypeDefs.h"

/** Implementation of the IMemberTypeDescriptor for a statically known type (e.g. a primitive like int32, FString, float, double) */
template<typename T>
class TMemberTypeDescriptor : public IMemberTypeDescriptor {
protected:
    std::wstring TypeNameReference;
public:
    explicit TMemberTypeDescriptor( std::wstring InTypeName ) : TypeNameReference(std::move(InTypeName)) {}

    const T* GetValuePtr( const void* data ) const { return static_cast<const T*>( data ); };
    T* GetValuePtr( void* data ) const { return static_cast<T*>( data ); };

    std::wstring GetTypeName() const override { return TypeNameReference; }
    int32_t GetMemberSize() const override { return sizeof(T); }
    int32_t GetMemberAlignment() const override { return alignof(T); }
    void InitializeValue(void* data) const override { new (data) T(); }
    void DestroyValue(void* data) const override { GetValuePtr(data)->~T(); }
    void CopyValue(void* dest, const void* src) const override { *GetValuePtr(dest) = *GetValuePtr(src); }
    void ClearValue(void* data) const override { *GetValuePtr(data) = T(); }

    static TMemberTypeDescriptor* StaticDescriptor( const wchar_t* TypeName )
    {
        static TMemberTypeDescriptor StaticDescriptor{ TypeName };
        return &StaticDescriptor;
    }
};

/** Member type descriptor for the dynamic type */
class DTL_API FDynamicTypeDescriptor : public IMemberTypeDescriptor
{
protected:
    IDynamicTypeInterface* DynamicType{};
public:
    explicit FDynamicTypeDescriptor( IDynamicTypeInterface* InDynamicType );

    std::wstring GetTypeName() const override;
    int32_t GetMemberSize() const override;
    int32_t GetMemberAlignment() const override;
    void InitializeValue(void* data) const override;
    void DestroyValue(void* data) const override;
    void CopyValue(void* dest, const void* src) const override;
    void ClearValue(void* data) const override;
    IDynamicTypeInterface * GetDynamicType() const override;

    static FDynamicTypeDescriptor* FindOrCreateTypeDescriptor( IDynamicTypeInterface* InDynamicType );
};

class ExternalDynamicTypeInternal;

/** Empty type is a type with no members */
class DTL_API EmptyDynamicType : public IDynamicTypeInterface
{
protected:
    std::wstring TypeName;
public:
    explicit EmptyDynamicType( std::wstring InTypeName ) : TypeName(std::move( InTypeName )) {}

    void RegisterTypeMember( IDynamicTypeMember* InTypeMember ) override;
    void GetAllTypeMembers(std::vector<IDynamicTypeMember *> &OutTypeMembers) const override {}
    IDynamicTypeMember * FindTypeMember(const std::wstring &MemberName) const override { return nullptr; }
    bool InitializeTypeInstance(void* Instance, int32_t InternalFlags) const override { return true; }
    void DestroyTypeInstance(void* Instance, int32_t InternalFlags) const override {}
    void CopyTypeInstance(void* DestInstance, const void* SrcInstance, int32_t InternalFlags) const override {}
    void GetLayoutDependencies(std::vector<IDynamicTypeInterface*>& LayoutDependencies) const override {}
    bool IsLayoutComputed() const override { return true; }
    bool RecomputeLayout(bool bForce) override { return true; }
    int32_t GetSize() const override { return 0; }
    int32_t GetMinAlignment() const override { return 0; }
};

/** External dynamic type is a dynamic type which member offsets are sources externally, and also initialized and copied externally. */
class DTL_API FExternalLayoutDynamicType : public IDynamicTypeInterface {
protected:
    std::unique_ptr<ExternalDynamicTypeInternal> InternalState{};
public:
    FExternalLayoutDynamicType( const std::wstring& InTypeName );

    void SetupExternalType( int32_t InTypeSize, int32_t InAlignment );
    void AddExternalMemberOffset( IDynamicTypeMember* Member, int32_t InPredefinedMemberOffset );

    void RegisterTypeMember( IDynamicTypeMember* InTypeMember ) override;
    void GetAllTypeMembers(std::vector<IDynamicTypeMember *> &OutTypeMembers) const override;
    IDynamicTypeMember * FindTypeMember(const std::wstring &MemberName) const override;
    bool InitializeTypeInstance(void* Instance, int32_t InternalFlags) const override;
    void DestroyTypeInstance(void* Instance, int32_t InternalFlags) const override;
    void CopyTypeInstance(void* DestInstance, const void* SrcInstance, int32_t InternalFlags) const override;
    void GetLayoutDependencies(std::vector<IDynamicTypeInterface*>& LayoutDependencies) const override;
    bool IsLayoutComputed() const override;
    bool RecomputeLayout(bool bForce) override;
    int32_t GetSize() const override;
    int32_t GetMinAlignment() const override;
};

class CustomDynamicTypeInternal;

/** Sequential dynamic type is a dynamic type which properties are defined locally and their offsets should be computed and members initialized */
class DTL_API FSequentialLayoutDynamicType : public IDynamicTypeInterface {
protected:
    std::unique_ptr<CustomDynamicTypeInternal> InternalState{};
public:
    FSequentialLayoutDynamicType( const std::wstring& InTypeName );

    void RegisterTypeMember( IDynamicTypeMember* InTypeMember ) override;
    void GetAllTypeMembers(std::vector<IDynamicTypeMember *> &OutTypeMembers) const override;
    IDynamicTypeMember * FindTypeMember(const std::wstring &MemberName) const override;
    bool InitializeTypeInstance(void* Instance, int32_t InternalFlags) const override;
    void DestroyTypeInstance(void* Instance, int32_t InternalFlags) const override;
    void CopyTypeInstance(void* DestInstance, const void* SrcInstance, int32_t InternalFlags) const override;
    void GetLayoutDependencies(std::vector<IDynamicTypeInterface*>& LayoutDependencies) const override;
    bool IsLayoutComputed() const override;
    bool RecomputeLayout(bool bForce) override;
    int32_t GetSize() const override;
    int32_t GetMinAlignment() const override;
};

using CollectTypeMembersFunc = void(*)(std::vector<IDynamicTypeMember*>&);

DTL_API IDynamicTypeInterface* GetPrivateStaticTypeFSequentialLayoutDynamicType( IDynamicTypeInterface** OutDynamicType, const wchar_t* InTypeName, IDynamicTypeInterface* InParentType, CollectTypeMembersFunc InCollectDynamicMembers, uint32_t ExtraFlags = 0 );
DTL_API IDynamicTypeInterface* GetPrivateStaticTypeFExternalLayoutDynamicType( IDynamicTypeInterface** OutDynamicType, const wchar_t* InTypeName, IDynamicTypeInterface* InParentType, CollectTypeMembersFunc InCollectDynamicMembers, uint32_t ExtraFlags = 0 );
