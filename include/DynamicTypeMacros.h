#pragma once

#include "DynamicTypeDefs.h"
#include "DynamicTypeTraits.h"

/// Declares a dynamic type with the specified parameters. Has to be followed by END_DYNAMIC_TYPE
#define DYNAMIC_TYPE_BODY( __TYPE_NAME__, __PARENT_TYPE__, __API_MACRO__ )    \
        public:                                                               \
            static constexpr bool IsDynamicType = true;                       \
            /** A couple of typedefs to be able to reference the current class and the parent class */ \
            using ThisClass = __TYPE_NAME__;                                  \
            using ParentClass = __PARENT_TYPE__;                              \
                                                                              \
            /** Delete the default constructor, dynamic types cannot be constructed directly */ \
            explicit __TYPE_NAME__() = delete;                                \
            __TYPE_NAME__( const __TYPE_NAME__& ) = delete;                   \
            __TYPE_NAME__( __TYPE_NAME__&& ) = delete;                        \
                                                                              \
            /** @return the dynamic type corresponding to this class */       \
            static IDynamicTypeInterface* StaticType();                       \
            static const wchar_t* StaticTypeName() { return DTL_TEXT(#__TYPE_NAME__); } \
        private:                                                              \
            static constexpr uint64_t FirstMemberIndex = __COUNTER__;         \
            template<uint64_t MemberIndex>                                    \
            static void __CollectDynamicMembers(std::vector<IDynamicTypeMember*>& out_members) \
                {                                                             \
                    abort(); /** This function should NEVER be called directly as a non specialized variant */ \
                }                                                             \
            template<>                                                        \
            void __CollectDynamicMembers<FirstMemberIndex>(std::vector<IDynamicTypeMember*>& out_members) \
                {                                                             \
                    __CollectDynamicMembers<FirstMemberIndex + 1>(out_members); /** On the first function we just call the first real member */ \
                }                                                             \


/// Closes the dynamic type declared using BEGIN_DYNAMIC_TYPE
#define DYNAMIC_TYPE_END   \
        private:           \
            static constexpr uint64_t LastMemberIndex = __COUNTER__; \
            template<>     \
            static void __CollectDynamicMembers<LastMemberIndex>(std::vector<IDynamicTypeMember*>& out_members) \
            {              \
                /** This one is the last one called and does not need to add anything */ \
            }              \
            static void CollectDynamicMembers(std::vector<IDynamicTypeMember*>& out_members) \
            {              \
                __CollectDynamicMembers<FirstMemberIndex>(out_members);                      \
            }              \

// This one does declare most of the boilerplate for the dynamic member, but does not define ConstructDynamicMember_MemberName
#define DEFINE_DYNAMIC_MEMBER_BOILERPLATE( __MEMBER_NAME__ ) \
        private:                                             \
            static constexpr uint64_t MemberIndex_##__MEMBER_NAME__ = __COUNTER__; \
            template<>     \
            static void __CollectDynamicMembers<MemberIndex_##__MEMBER_NAME__>(std::vector<IDynamicTypeMember*>& out_members) \
            {              \
                out_members.push_back(ConstructDynamicMember_##__MEMBER_NAME__());       \
                __CollectDynamicMembers<MemberIndex_##__MEMBER_NAME__ + 1>(out_members); \
            }                                                \

#define DEFINE_DYNAMIC_TYPE_MEMBER( __MEMBER_CLASS__, __MEMBER_NAME__, ... ) \
        private:                                                             \
            static IDynamicTypeMember* ConstructDynamicMember_##__MEMBER_NAME__() \
            {                                                                \
                static __MEMBER_CLASS__ StaticMemberInstance{ DTL_TEXT( #__MEMBER_NAME__ ), __VA_ARGS__ };     \
                return &StaticMemberInstance;                                \
            }                                                                \
                                                                             \
            static IDynamicTypeMember* GetDynamicMember_##__MEMBER_NAME__()  \
            {                                                                \
                static IDynamicTypeMember* StaticMemberInstance = StaticType()->FindTypeMember( DTL_TEXT( #__MEMBER_NAME__ ) ); \
                return StaticMemberInstance;                                 \
            }                                                                \
        DEFINE_DYNAMIC_MEMBER_BOILERPLATE( __MEMBER_NAME__ );                \


#define DEFINE_OPTIONAL_TYPE_MEMBER_FULL( __ACCESS_SPECIFIER__, __MEMBER_CLASS__, __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
        DEFINE_DYNAMIC_TYPE_MEMBER( __MEMBER_CLASS__, __MEMBER_NAME__, TMemberTypeDescriptorProvider<__MEMBER_TYPE__>::MakeTypeDescriptor( DTL_TEXT( #__MEMBER_TYPE__ ) ), __VA_ARGS__ ) \
    __ACCESS_SPECIFIER__:                                                                                                 \
        TMemberTypeDescriptorProvider<__MEMBER_TYPE__>::ValuePtrType Get##__MEMBER_NAME__##Ptr()                          \
        {                                                                                                                 \
            return TMemberTypeDescriptorProvider<__MEMBER_TYPE__>::MakeValuePointer( GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<void>( this ) ); \
        }                                                                                                                 \
        TMemberTypeDescriptorProvider<__MEMBER_TYPE__>::ValueConstPtrType Get##__MEMBER_NAME__##Ptr() const               \
        {                                                                                                                 \
            return TMemberTypeDescriptorProvider<__MEMBER_TYPE__>::MakeConstValuePointer( GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<void>( this ) ); \
        }                                                                                                                 \

#define DEFINE_OPTIONAL_TYPE_MEMBER( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_OPTIONAL_TYPE_MEMBER_FULL( public, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, true )

#define DEFINE_OPTIONAL_TYPE_MEMBER_PRIVATE( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_OPTIONAL_TYPE_MEMBER_FULL( private, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, true )

#define DEFINE_OPTIONAL_TYPE_MEMBER_PROTECTED( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_OPTIONAL_TYPE_MEMBER_FULL( protected, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, true )

#define DEFINE_TYPE_MEMBER_FULL( __ACCESS_SPECIFIER__, __MEMBER_CLASS__, __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_OPTIONAL_TYPE_MEMBER_FULL( __ACCESS_SPECIFIER__, __MEMBER_CLASS__, __MEMBER_TYPE__, __MEMBER_NAME__, __VA_ARGS__ ) \
    __ACCESS_SPECIFIER__:                                                                                             \
        __MEMBER_TYPE__& Get##__MEMBER_NAME__()                                                                       \
        {                                                                                                             \
            return *Get##__MEMBER_NAME__##Ptr();                                                                      \
        }                                                                                                             \
        const __MEMBER_TYPE__& Get##__MEMBER_NAME__() const                                                           \
        {                                                                                                             \
            /* We cannot use const variant of GetMemberNamePtr here because it would return a copy that would immediately go out of scope when function returns */ \
            return const_cast<ThisClass*>( this )->Get##__MEMBER_NAME__();                                            \
        }                                                                                                             \
        void Set##__MEMBER_NAME__( const __MEMBER_TYPE__& InNewValue )                                                \
        {                                                                                                             \
            TMemberTypeDescriptorProvider<__MEMBER_TYPE__>::CopyValue( const_cast<ThisClass*>( this )->Get##__MEMBER_NAME__##Ptr(), &InNewValue ); \
        }                                                                                                             \

#define DEFINE_TYPE_MEMBER( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_FULL( public, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_PRIVATE( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_FULL( private, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_PROTECTED( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_FULL( protected, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define IMPLEMENT_DYNAMIC_TYPE_FULL( __DYNAMIC_TYPE_CLASS__, __TYPE_NAME__, ... ) \
    IDynamicTypeInterface* __TYPE_NAME__::StaticType()                            \
    {                                                                             \
        static IDynamicTypeInterface* PrivateStaticType{};                        \
        if ( !PrivateStaticType )                                                 \
        {                                                                         \
            GetPrivateStaticType##__DYNAMIC_TYPE_CLASS__( &PrivateStaticType, StaticTypeName(), ParentClass::StaticType(), &__TYPE_NAME__::CollectDynamicMembers , __VA_ARGS__ ); \
        }                                                                         \
        return PrivateStaticType;                                                 \
    }

#define IMPLEMENT_DYNAMIC_TYPE_SEQUENTIAL( __TYPE_NAME__ ) \
    IMPLEMENT_DYNAMIC_TYPE_FULL( FSequentialLayoutDynamicType, __TYPE_NAME__, 0 )

#define IMPLEMENT_DYNAMIC_TYPE_EXTERNAL( __TYPE_NAME__ ) \
    IMPLEMENT_DYNAMIC_TYPE_FULL( FExternalLayoutDynamicType, __TYPE_NAME__, 0 )
