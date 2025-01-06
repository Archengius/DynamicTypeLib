#pragma once

#include "DynamicTypeDefs.h"
#include "DynamicTypeTraits.h"

/// Declares a dynamic type without any members. This can be used to declare a minimal dynamic type
#define DECLARE_DYNAMIC_TYPE( __TYPE_NAME__, __PARENT_TYPE__, __API_MACRO__ ) \
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
        static __API_MACRO__ IDynamicTypeLayout* StaticType();         \
        static const DTL_CHAR* StaticTypeName() { return DTL_TEXT(#__TYPE_NAME__); } \
        \
        /** Define copy assignment operator for this type and also for the Dyn reference to this type for convenience */ \
        ThisClass& operator=(const ThisClass& Other) { AssignDynamicType(*this, Other); return *this; } \
        ThisClass& operator=(const Dyn<ThisClass>& Other) { AssignDynamicType(*this, *Other); return *this; } \

/// Declares a dynamic type with the specified parameters. Has to be followed by END_DYNAMIC_TYPE
#define DYNAMIC_TYPE_BODY( __TYPE_NAME__, __PARENT_TYPE__, __API_MACRO__ )     \
        DECLARE_DYNAMIC_TYPE( __TYPE_NAME__, __PARENT_TYPE__, __API_MACRO__ ); \
        private:                                                               \
            static constexpr uint64_t FirstMemberIndex = __COUNTER__;          \
            template<uint64_t MemberIndex>                                     \
            static void __CollectDynamicMembers(std::vector<FDynamicTypeMember*>& OutMembers, std::vector<FDynamicTypeVirtualFunction*>& OutVirtualFunctions) \
                {                                                              \
                    abort(); /** This function should NEVER be called directly as a non-specialized variant */ \
                }                                                              \
            template<>                                                         \
            void __CollectDynamicMembers<FirstMemberIndex>(std::vector<FDynamicTypeMember*>& OutMembers, std::vector<FDynamicTypeVirtualFunction*>& OutVirtualFunctions) \
                {                                                              \
                    __CollectDynamicMembers<FirstMemberIndex + 1>(OutMembers, OutVirtualFunctions); /** On the first function we just call the first real member */ \
                }                                                              \

/// Closes the dynamic type declared using BEGIN_DYNAMIC_TYPE
#define DYNAMIC_TYPE_END   \
        private:           \
            static constexpr uint64_t LastMemberIndex = __COUNTER__; \
            template<>     \
            static void __CollectDynamicMembers<LastMemberIndex>(std::vector<FDynamicTypeMember*>& OutMembers, std::vector<FDynamicTypeVirtualFunction*>& OutVirtualFunctions) \
            {              \
                /** This one is the last one called and does not need to add anything */ \
            }              \
            static void CollectDynamicMembers(std::vector<FDynamicTypeMember*>& OutMembers, std::vector<FDynamicTypeVirtualFunction*>& OutVirtualFunctions) \
            {              \
                __CollectDynamicMembers<FirstMemberIndex>(OutMembers, OutVirtualFunctions); \
            }              \

// This one does declare most of the boilerplate for the dynamic member, but does not define ConstructDynamicMember_MemberName
#define DEFINE_DYNAMIC_MEMBER_BOILERPLATE( __MEMBER_NAME__ ) \
        private:                                             \
            static constexpr uint64_t MemberIndex_##__MEMBER_NAME__ = __COUNTER__; \
            template<>     \
            static void __CollectDynamicMembers<MemberIndex_##__MEMBER_NAME__>(std::vector<FDynamicTypeMember*>& OutMembers, std::vector<FDynamicTypeVirtualFunction*>& OutVirtualFunctions) \
            {              \
                OutMembers.push_back(ConstructDynamicMember_##__MEMBER_NAME__());       \
                __CollectDynamicMembers<MemberIndex_##__MEMBER_NAME__ + 1>(OutMembers, OutVirtualFunctions); \
            }                                                \

#define DEFINE_DYNAMIC_VIRTUAL_FUNCTION_BOILERPLATE( __VIRTUAL_FUNCTION_NAME__ ) \
        private:                                             \
            static constexpr uint64_t MemberIndex_##__VIRTUAL_FUNCTION_NAME__ = __COUNTER__; \
            template<>     \
            static void __CollectDynamicMembers<MemberIndex_##__VIRTUAL_FUNCTION_NAME__>(std::vector<FDynamicTypeMember*>& OutMembers, std::vector<FDynamicTypeVirtualFunction*>& OutVirtualFunctions) \
            {              \
            OutVirtualFunctions.push_back(ConstructDynamicVirtualFunction_##__VIRTUAL_FUNCTION_NAME__());       \
            __CollectDynamicMembers<MemberIndex_##__VIRTUAL_FUNCTION_NAME__ + 1>(OutMembers, OutVirtualFunctions); \
            }                                                \

#define DEFINE_DYNAMIC_TYPE_MEMBER( __MEMBER_CLASS__, __MEMBER_NAME__, ... ) \
        private:                                                             \
            static FDynamicTypeMember* ConstructDynamicMember_##__MEMBER_NAME__() \
            {                                                                \
                static __MEMBER_CLASS__ StaticMemberInstance{ DTL_TEXT( #__MEMBER_NAME__ ), __VA_ARGS__ };     \
                return &StaticMemberInstance;                                \
            }                                                                \
                                                                             \
            static FDynamicTypeMember* GetDynamicMember_##__MEMBER_NAME__()  \
            {                                                                \
                static FDynamicTypeMember* StaticMemberInstance = StaticType()->FindTypeMember( DTL_TEXT( #__MEMBER_NAME__ ) ); \
                return StaticMemberInstance;                                 \
            }                                                                \
        DEFINE_DYNAMIC_MEMBER_BOILERPLATE( __MEMBER_NAME__ );                \

#define DEFINE_DYNAMIC_TYPE_VIRTUAL_FUNCTION( __VIRTUAL_FUNCTION_CLASS__, __VIRTUAL_FUNCTION_NAME__, ... ) \
        private:                                                             \
            static FDynamicTypeVirtualFunction* ConstructDynamicVirtualFunction_##__VIRTUAL_FUNCTION_NAME__() \
            {                                                                \
            static __VIRTUAL_FUNCTION_CLASS__ StaticVirtualFunctionInstance{ DTL_TEXT( #__VIRTUAL_FUNCTION_NAME__ ), __VA_ARGS__ };     \
            return &StaticVirtualFunctionInstance;                           \
            }                                                                \
            \
            static FDynamicTypeVirtualFunction* GetDynamicVirtualFunction_##__VIRTUAL_FUNCTION_NAME__()  \
            {                                                                \
            static FDynamicTypeVirtualFunction* StaticVirtualFunctionInstance = StaticType()->FindVirtualFunction( DTL_TEXT( #__VIRTUAL_FUNCTION_NAME__ ) ); \
            return StaticVirtualFunctionInstance;                            \
            }                                                                \
            DEFINE_DYNAMIC_VIRTUAL_FUNCTION_BOILERPLATE( __VIRTUAL_FUNCTION_NAME__ );                \

#define DEFINE_TYPE_MEMBER_PTR_FULL( __ACCESS_SPECIFIER__, __MEMBER_CLASS__, __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
        DEFINE_DYNAMIC_TYPE_MEMBER( __MEMBER_CLASS__, __MEMBER_NAME__, StaticMemberType<__MEMBER_TYPE__>( DTL_TEXT( #__MEMBER_TYPE__ ) ), __VA_ARGS__ ) \
    __ACCESS_SPECIFIER__:                                                                                                 \
        __MEMBER_TYPE__* Get##__MEMBER_NAME__##Ptr()                                                                      \
        {                                                                                                                 \
            return GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<__MEMBER_TYPE__>(this);                   \
        }                                                                                                                 \
        const __MEMBER_TYPE__* Get##__MEMBER_NAME__##Ptr() const                                                          \
        {                                                                                                                 \
            return GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<__MEMBER_TYPE__>(this);                   \
        }                                                                                                                 \

#define DEFINE_OPTIONAL_TYPE_MEMBER( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_PTR_FULL( public, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, true )

#define DEFINE_OPTIONAL_TYPE_MEMBER_PRIVATE( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_PTR_FULL( private, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, true )

#define DEFINE_OPTIONAL_TYPE_MEMBER_PROTECTED( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_PTR_FULL( protected, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, true )

#define DEFINE_TYPE_MEMBER_BY_REF_FULL( __ACCESS_SPECIFIER__, __MEMBER_CLASS__, __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
        DEFINE_DYNAMIC_TYPE_MEMBER( __MEMBER_CLASS__, __MEMBER_NAME__, StaticMemberType<__MEMBER_TYPE__>( DTL_TEXT( #__MEMBER_TYPE__ ) ), __VA_ARGS__ ) \
    __ACCESS_SPECIFIER__:                                                                                             \
        __MEMBER_TYPE__& Get##__MEMBER_NAME__()                                                                       \
        {                                                                                                             \
            return *GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<__MEMBER_TYPE__>(this);              \
        }                                                                                                             \
        const __MEMBER_TYPE__& Get##__MEMBER_NAME__() const                                                           \
        {                                                                                                             \
            return *GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<__MEMBER_TYPE__>(this);              \
        }                                                                                                             \

#define DEFINE_TYPE_MEMBER_BY_VAL_FULL( __ACCESS_SPECIFIER__, __MEMBER_CLASS__, __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
        DEFINE_DYNAMIC_TYPE_MEMBER( __MEMBER_CLASS__, __MEMBER_NAME__, StaticMemberType<__MEMBER_TYPE__>( DTL_TEXT( #__MEMBER_TYPE__ ) ), __VA_ARGS__ ) \
    __ACCESS_SPECIFIER__:                                                                                             \
        __MEMBER_TYPE__ Get##__MEMBER_NAME__() const                                                                  \
        {                                                                                                             \
            return *GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<__MEMBER_TYPE__>(this);              \
        }                                                                                                             \
        void Set##__MEMBER_NAME__(__MEMBER_TYPE__ InNewValue)                                                         \
        {                                                                                                             \
            *GetDynamicMember_##__MEMBER_NAME__()->ContainerPtrToValuePtr<__MEMBER_TYPE__>(this) = InNewValue;        \
        }                                                                                                             \

#define DEFINE_TYPE_MEMBER_REF( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_BY_REF_FULL( public, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_REF_PRIVATE( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_BY_REF_FULL( private, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_REF_PROTECTED( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_BY_REF_FULL( protected, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_VAL( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_BY_VAL_FULL( public, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_VAL_PRIVATE( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_BY_VAL_FULL( private, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define DEFINE_TYPE_MEMBER_VAL_PROTECTED( __MEMBER_TYPE__, __MEMBER_NAME__, ... ) \
    DEFINE_TYPE_MEMBER_BY_VAL_FULL( protected, FDynamicTypeMember, __MEMBER_TYPE__, __MEMBER_NAME__, false )

#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_()
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_1(Type1, Value1) Type1 Value1
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_2(Type1, Value1, Type2, Value2) Type1 Value1, Type2 Value2
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_3(Type1, Value1, Type2, Value2, Type3, Value3) Type1 Value1, Type2 Value2, Type3 Value3
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_4(Type1, Value1, Type2, Value2, Type3, Value3, Type4, Value4) Type1 Value1, Type2 Value2, Type3 Value3, Type4 Value4
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_5(Type1, Value1, Type2, Value2, Type3, Value3, Type4, Value4, Type5, Value5) Type1 Value1, Type2 Value2, Type3 Value3, Type4 Value4, Type5 Value5
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_6(Type1, Value1, Type2, Value2, Type3, Value3, Type4, Value4, Type5, Value5, Type6, Value6) Type1 Value1, Type2 Value2, Type3 Value3, Type4 Value4, Type5 Value5, Type6 Value6

#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_()
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_1(Type1, Value1) Value1
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_2(Type1, Value1, Type2, Value2) Value1, Value2
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_3(Type1, Value1, Type2, Value2, Type3, Value3) Value1, Value2, Value3
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_4(Type1, Value1, Type2, Value2, Type3, Value3, Type4, Value4) Value1, Value2, Value3, Value4
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_5(Type1, Value1, Type2, Value2, Type3, Value3, Type4, Value4, Type5, Value5) Value1, Value2, Value3, Value4, Value5
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_6(Type1, Value1, Type2, Value2, Type3, Value3, Type4, Value4, Type5, Value5, Type6, Value6) Value1, Value2, Value3, Value4, Value5, Value6

#define GET_VIRTUAL_FUNCTION_ARGUMENTS_MACRO(__MACRO_NAME__, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, __MACRO_SUFFIX__, ...) __MACRO_NAME__##__MACRO_SUFFIX__
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL(...) GET_VIRTUAL_FUNCTION_ARGUMENTS_MACRO(PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL_, __VA_ARGS__, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1)(__VA_ARGS__)
#define PASTE_VIRTUAL_FUNCTION_ARGUMENTS(...) GET_VIRTUAL_FUNCTION_ARGUMENTS_MACRO(PASTE_VIRTUAL_FUNCTION_ARGUMENTS_, __VA_ARGS__, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1)(__VA_ARGS__)

#define DEFINE_VIRTUAL_FUNCTION_FULL( __ACCESS_SPECIFIER__, __FUNCTION_MODIFIERS__, __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) \
    DEFINE_DYNAMIC_TYPE_VIRTUAL_FUNCTION( FDynamicTypeVirtualFunction, __VIRTUAL_FUNCTION_NAME__, false ) \
    __ACCESS_SPECIFIER__: \
        TMemberVirtualFunctionReturnTypeProvider<__RETURN_TYPE__>::ReturnValueType __VIRTUAL_FUNCTION_NAME__(PASTE_VIRTUAL_FUNCTION_ARGUMENTS_DECL(__VA_ARGS__)) __FUNCTION_MODIFIERS__ \
        { \
            static FDynamicTypeVirtualFunction* StaticVirtualFunction = GetDynamicVirtualFunction_##__VIRTUAL_FUNCTION_NAME__(); \
            GenericFunctionPtr VirtualFunctionPointer = StaticVirtualFunction->ContainerPtrToVirtualFunctionPtr(this); \
            if constexpr(!std::is_void_v<__RETURN_TYPE__>) \
            { \
                return TMemberVirtualFunctionInvoker<decltype(&ThisClass::__VIRTUAL_FUNCTION_NAME__)>::Invoke(VirtualFunctionPointer, this __VA_OPT__(,) PASTE_VIRTUAL_FUNCTION_ARGUMENTS(__VA_ARGS__)); \
            } \
            else \
            { \
                TMemberVirtualFunctionInvoker<decltype(&ThisClass::__VIRTUAL_FUNCTION_NAME__)>::Invoke(VirtualFunctionPointer, this __VA_OPT__(,) PASTE_VIRTUAL_FUNCTION_ARGUMENTS(__VA_ARGS__)); \
            } \
        }

#define DEFINE_VIRTUAL_FUNCTION( __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) DEFINE_VIRTUAL_FUNCTION_FULL(public, , __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, __VA_ARGS__)
#define DEFINE_VIRTUAL_FUNCTION_PRIVATE( __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) DEFINE_VIRTUAL_FUNCTION_FULL(private, , __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, __VA_ARGS__)
#define DEFINE_VIRTUAL_FUNCTION_PROTECTED( __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) DEFINE_VIRTUAL_FUNCTION_FULL(protected, , __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, __VA_ARGS__)
#define DEFINE_CONST_VIRTUAL_FUNCTION( __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) DEFINE_VIRTUAL_FUNCTION_FULL(public, const, __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, __VA_ARGS__)
#define DEFINE_CONST_VIRTUAL_FUNCTION_PRIVATE( __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) DEFINE_VIRTUAL_FUNCTION_FULL(private, const, __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, __VA_ARGS__)
#define DEFINE_CONST_VIRTUAL_FUNCTION_PROTECTED( __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, ... ) DEFINE_VIRTUAL_FUNCTION_FULL(protected, const, __VIRTUAL_FUNCTION_NAME__, __RETURN_TYPE__, __VA_ARGS__)

#define IMPLEMENT_DYNAMIC_TYPE_FULL( __DYNAMIC_TYPE_CLASS__, __TYPE_NAME__, ... ) \
    IDynamicTypeLayout* __TYPE_NAME__::StaticType()                            \
    {                                                                             \
        static std::unique_ptr<TypeImplClass> PrivateStaticType = ConstructPrivateStaticType<__DYNAMIC_TYPE_CLASS__>(StaticTypeName(), ParentClass::StaticType(), &__TYPE_NAME__::CollectDynamicMembers, __VA_ARGS__); \                                                        \
        return PrivateStaticType.get();                                                 \
    }

#define IMPLEMENT_DYNAMIC_TYPE_SEQUENTIAL( __TYPE_NAME__ ) \
    IMPLEMENT_DYNAMIC_TYPE_FULL( FSequentialLayoutDynamicType, __TYPE_NAME__, 0 )

#define IMPLEMENT_DYNAMIC_TYPE_EXTERNAL( __TYPE_NAME__ ) \
    IMPLEMENT_DYNAMIC_TYPE_FULL( FExternalLayoutDynamicType, __TYPE_NAME__, 0 )
