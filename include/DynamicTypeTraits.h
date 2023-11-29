#pragma once

#include "DynamicTypeImpl.h"

template<typename T>
struct TIsDynamicType
{
    static constexpr bool Value = false;
};

template<typename T>
requires(T::IsDynamicType)
struct TIsDynamicType<T>
{
    static constexpr bool Value = true;
};

/// Returns true if T is a dynamic type
template<typename T>
constexpr bool TIsDynamicTypeValue = TIsDynamicType<T>::Value;

// Default implementation for static types
template<typename T>
struct TMemberTypeDescriptorProvider
{
    // Pointer types for values
    using ValuePtrType = T*;
    using ValueConstPtrType = const T*;

    static void CopyValue( T* DestDataPtr, const T* SrcDataPtr )
    {
        *DestDataPtr = *SrcDataPtr;
    }

    static ValuePtrType MakeValuePointer( void* ValueDataPtr )
    {
        return static_cast<T*>( ValueDataPtr );
    }

    static ValueConstPtrType MakeConstValuePointer( const void* ValueDataPtr )
    {
        return static_cast<const T*>( ValueDataPtr );
    }

    static IMemberTypeDescriptor* MakeTypeDescriptor( const wchar_t* InMemberTypeName )
    {
        static_assert( !TIsDynamicTypeValue<T> );
        return TMemberTypeDescriptor<T>::StaticDescriptor( InMemberTypeName );
    }
};

// Specialization for dynamic types
template<typename T>
requires( TIsDynamicTypeValue<T> )
struct TMemberTypeDescriptorProvider<T>
{
    // Pointer types for values
    using ValuePtrType = TDynamicTypeInstance<T>;
    using ValueConstPtrType = TDynamicTypeInstance<T>;

    static void CopyValue( ValuePtrType DestDataPtr, const T* SrcDataPtr )
    {
        MakeTypeDescriptor( nullptr )->CopyValue( DestDataPtr.Get(), SrcDataPtr );
    }

    static ValuePtrType MakeValuePointer( void* ValueDataPtr )
    {
        return TDynamicTypeInstance<T>( ValueDataPtr );
    }

    static ValueConstPtrType MakeConstValuePointer( const void* ValueDataPtr )
    {
        return TDynamicTypeInstance<T>( ValueDataPtr );
    }

    static IMemberTypeDescriptor* MakeTypeDescriptor( const wchar_t* )
    {
        return FDynamicTypeDescriptor::FindOrCreateTypeDescriptor( T::StaticType() );
    }
};

