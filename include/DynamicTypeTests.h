#pragma once

#include "DynamicTypeMacros.h"

struct DTL_API FMoveComponentParams : public FDynamicTypeBase
{
    DYNAMIC_TYPE_BODY( FMoveComponentParams, FDynamicTypeBase, DTL_API )

    DEFINE_OPTIONAL_TYPE_MEMBER( int32_t, ExampleOptionalInt32 )
    DEFINE_TYPE_MEMBER( int32_t, ExampleInt32 )

    DYNAMIC_TYPE_END
};

struct DTL_API FNestedDynamicType : public FDynamicTypeBase
{
    DYNAMIC_TYPE_BODY( FNestedDynamicType, FDynamicTypeBase, DTL_API )

    DEFINE_OPTIONAL_TYPE_MEMBER( wchar_t, ExampleCharBefore )
    DEFINE_OPTIONAL_TYPE_MEMBER( FMoveComponentParams, ExampleNestedOptional )
    DEFINE_TYPE_MEMBER( FMoveComponentParams, ExampleNested )
    DEFINE_TYPE_MEMBER( uint8_t, LastExample )

    DYNAMIC_TYPE_END
};

struct DTL_API FInheritedDynamicType : public FMoveComponentParams
{
    DYNAMIC_TYPE_BODY( FInheritedDynamicType, FMoveComponentParams, DTL_API )

    DEFINE_TYPE_MEMBER( uint32_t, ChildMember );

    DYNAMIC_TYPE_END
};

void TestDynamicType() {
    TDynamicTypeInstance<FMoveComponentParams> ExampleParams;

    ExampleParams->GetExampleOptionalInt32Ptr();
    ExampleParams->SetExampleInt32( ExampleParams->GetExampleInt32() );

    TDynamicTypeInstance<FNestedDynamicType> NestedTypeInstance;
    NestedTypeInstance->GetExampleCharBeforePtr();
    NestedTypeInstance->GetExampleNestedOptionalPtr()->GetExampleInt32Ptr();
    NestedTypeInstance->GetExampleNested().SetExampleInt32( 1 );
    NestedTypeInstance->SetExampleNested( NestedTypeInstance->GetExampleNested() );
    NestedTypeInstance->SetLastExample( 5 );

    TDynamicTypeInstance<FInheritedDynamicType> ChildInstance;
    ChildInstance->GetExampleInt32Ptr();
    ChildInstance->SetChildMember( 6 );
}
