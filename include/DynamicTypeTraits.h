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

/** Provides descriptor for a type of the member variable. Descriptor allows manipulating the value of the type regardless of whenever it's a dynamic type or not */
template<typename InMemberType>
IMemberTypeDescriptor* StaticMemberType(const DTL_CHAR* InTypeName)
{
    static TMemberTypeDescriptor<InMemberType> StaticTypeDescriptor(InTypeName);
    return &StaticTypeDescriptor;
}

/** Implementation of member type descriptor for dynamic types */
template<typename InMemberType>
requires(TIsDynamicTypeValue<InMemberType>)
IMemberTypeDescriptor* StaticMemberType(const DTL_CHAR*)
{
    static FDynamicMemberTypeDescriptor StaticTypeDescriptor(InMemberType::StaticType());
    return &StaticTypeDescriptor;
}

/** Default no-argument constructor for dynamic types. Compiles for all dynamic types, but the dynamic type has to implement EmplaceTypeInstance */
template<typename InDynamicType>
void EmplaceDynamicType(InDynamicType* PlacementStorage)
{
    static IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
    StaticType->EmplaceTypeInstance(PlacementStorage);
}

/** Default copy constructor for dynamic types. Compiles for all dynamic types, but the dynamic type has to implement CopyAssignTypeInstance */
template<typename InDynamicType>
void EmplaceDynamicType(InDynamicType* PlacementStorage, const InDynamicType& Other)
{
    static IDynamicTypeLayout* StaticType = InDynamicType::StaticType();

    // Note that this implementation is not efficient, because it constructs a new instance and then immediately overwrites it with a copy
    StaticType->EmplaceTypeInstance(PlacementStorage);
    StaticType->CopyAssignTypeInstance(PlacementStorage, &Other);
}

/** Default copy assignment operator for dynamic types. Compiles for all dynamic types, but the dynamic type has to implement CopyAssignTypeInstance */
template<typename InDynamicType>
void AssignDynamicType(InDynamicType& DynamicType, const InDynamicType& Other)
{
    static IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
    StaticType->CopyAssignTypeInstance(&DynamicType, &Other);
}

/** Destroys the dynamic type stored at the provided pointer */
template<typename InDynamicType>
void DestroyDynamicType(InDynamicType* InTypeStorage)
{
    static IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
    StaticType->DestructTypeInstance(InTypeStorage);
}

/**
 * Dyn is a container that holds an instance of a dynamic type allocated on the heap
 * This is the type that is used when you want to construct a value of the dynamic type, and that should be used as a constructor for a dynamic type
 * There are convenience functions defined to freely convert between raw references to the dynamic type and Dyn containers
 * However, please note that this type is an indirect container, and not a type itself.
 * Note that this type always owns the memory and the data it holds, and will release both when it goes out of scope.
 */
template<typename InDynamicType>
class Dyn
{
    InDynamicType* TypeStorage{};
public:
    /** Constructs a new default-initialized instance of the dynamic type */
    Dyn()
    {
        const IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
        TypeStorage = malloc(StaticType->GetSize());
        StaticType->EmplaceTypeInstance(TypeStorage);
    }

    /** Move constructor for Dyn instance. Leaves other type in an invalid null-state */
    Dyn(Dyn&& Other) noexcept
    {
        TypeStorage = Other.TypeStorage;
        Other.TypeStorage = nullptr;
    }

    /** Destructor for Dyn. Will call the destructor of the underlying type and free the memory */
    ~Dyn()
    {
        if (TypeStorage)
        {
            DestroyDynamicType(TypeStorage);
            free(TypeStorage);
        }
    }

    /** Copy constructor for the Dyn instance */
    Dyn(const Dyn& Other) : Dyn()
    {
        if (Other.TypeStorage)
        {
            const IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
            StaticType->CopyAssignTypeInstance(TypeStorage, Other.TypeStorage);
        }
    }

    /** Constructs a Dyn instance from the raw reference to the dynamic type */
    Dyn(const InDynamicType& Other) : Dyn()
    {
        const IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
        StaticType->CopyAssignTypeInstance(TypeStorage, &Other);
    }

    /** Constructs a Dyn instance using the dynamic type-defined constructor */
    template<typename... InArgumentTypes>
    explicit Dyn(InArgumentTypes... InArgs)
    {
        const IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
        TypeStorage = malloc(StaticType->GetSize());
        EmplaceDynamicType<InDynamicType>(TypeStorage, std::forward<InArgumentTypes>(InArgs...));
    }

    /** Move assignment operator. Will use swap semantics for the move */
    Dyn& operator=(Dyn&& Other) noexcept
    {
        std::swap(TypeStorage, Other.TypeStorage);
        return *this;
    }

    /** Copy assignment operator */
    Dyn& operator=(const Dyn& Other)
    {
        if (TypeStorage && Other.TypeStorage)
        {
            const IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
            StaticType->CopyAssignTypeInstance(TypeStorage, Other.TypeStorage);
        }
        return *this;
    }

    /** Copy assignment operator for raw reference to a dynamic type */
    Dyn& operator=(const InDynamicType& Other)
    {
        if (TypeStorage)
        {
            const IDynamicTypeLayout* StaticType = InDynamicType::StaticType();
            StaticType->CopyAssignTypeInstance(TypeStorage, &Other);
        }
        return *this;
    }

    /** Copy assignment operator that delegates to the assignment operator defined on the dynamic type */
    template<typename InOtherType>
    Dyn& operator=(InOtherType Other)
    {
        if (TypeStorage)
        {
            *TypeStorage = Other;
        }
        return *this;
    }

    /** Implicit conversion operator to the reference to a dynamic type */
    operator InDynamicType&() { return *TypeStorage; }
    /** Implicit conversion operator to the const reference to a dynamic type */
    operator const InDynamicType&() const { return *TypeStorage; }

    /** Returns the reference to the contained dynamic type */
    InDynamicType& operator*() { return *TypeStorage; }
    /** Returns the reference to the contained dynamic type */
    const InDynamicType& operator*() const { return *TypeStorage; }

    /** Returns the pointer for accessing members of the contained dynamic type */
    InDynamicType* operator->() { return TypeStorage; }
    /** Returns the pointer for accessing members of the contained dynamic type */
    const InDynamicType* operator->() const { return TypeStorage; }
};

template<typename T>
struct TMemberVirtualFunctionReturnTypeProvider
{
    using ReturnValueType = T;
};
template<typename T>
requires(TIsDynamicTypeValue<T>)
struct TMemberVirtualFunctionReturnTypeProvider<T>
{
    using ReturnValueType = Dyn<T>;
};

template<typename TCallable>
struct TMemberVirtualFunctionInvoker;

/// Implementation for void-returning non-const member functions
template<typename TReceiverType, typename... TFunctionArguments>
struct TMemberVirtualFunctionInvoker<void(TReceiverType::*)(TFunctionArguments...)>
{
    static void Invoke(GenericFunctionPtr FunctionPtr, TReceiverType* Receiver, TFunctionArguments... Arguments)
    {
        using InvokeFunctionPtr = void(*)(TReceiverType*, TFunctionArguments...);
        reinterpret_cast<InvokeFunctionPtr>(FunctionPtr)(Receiver, Arguments...);
    }
};
/// Implementation for void-returning const member functions
template<typename TReceiverType, typename... TFunctionArguments>
struct TMemberVirtualFunctionInvoker<void(TReceiverType::*)(TFunctionArguments...) const>
{
    static void Invoke(GenericFunctionPtr FunctionPtr, const TReceiverType* Receiver, TFunctionArguments... Arguments)
    {
        using InvokeFunctionPtr = void(*)(const TReceiverType*, TFunctionArguments...);
        reinterpret_cast<InvokeFunctionPtr>(FunctionPtr)(Receiver, Arguments...);
    }
};
/// Implementation for non-dynamic-type-returning non-const member functions
template<typename TReceiverType, typename TReturnType, typename... TFunctionArguments>
requires(!TIsDynamicTypeValue<TReturnType>)
struct TMemberVirtualFunctionInvoker<TReturnType(TReceiverType::*)(TFunctionArguments...)>
{
    static TReturnType Invoke(GenericFunctionPtr FunctionPtr, TReceiverType* Receiver, TFunctionArguments... Arguments)
    {
        using InvokeFunctionPtr = TReturnType(*)(TReceiverType*, TFunctionArguments...);
        return reinterpret_cast<InvokeFunctionPtr>(FunctionPtr)(Receiver, Arguments...);
    }
};
/// Implementation for non-dynamic-type-returning const member functions
template<typename TReceiverType, typename TReturnType, typename... TFunctionArguments>
struct TMemberVirtualFunctionInvoker<TReturnType(TReceiverType::*)(TFunctionArguments...) const>
{
    static TReturnType Invoke(GenericFunctionPtr FunctionPtr, const TReceiverType* Receiver, TFunctionArguments... Arguments)
    {
        using InvokeFunctionPtr = TReturnType(*)(const TReceiverType*, TFunctionArguments...);
        return reinterpret_cast<InvokeFunctionPtr>(FunctionPtr)(Receiver, Arguments...);
    }
};
/// Implementation for dynamic-type-returning non-const member functions
template<typename TReceiverType, typename TReturnType, typename... TFunctionArguments>
struct TMemberVirtualFunctionInvoker<Dyn<TReturnType>(TReceiverType::*)(TFunctionArguments...)>
{
    static Dyn<TReturnType> Invoke(GenericFunctionPtr FunctionPtr, TReceiverType* Receiver, TFunctionArguments... Arguments)
    {
        static IDynamicTypeLayout* StaticReturnType = TReturnType::StaticType();
        using InvokeFunctionPtr = TReturnType*(*)(TReceiverType*, TReturnType*, TFunctionArguments...);

        // Allocate enough memory from the system allocator to hold the return value
        auto* ReturnValueStorage = static_cast<TReturnType*>(malloc(StaticReturnType->GetSize()));

        // Call the function pointer and pass it reference to the memory where return value should be written
        reinterpret_cast<InvokeFunctionPtr>(FunctionPtr)(Receiver, ReturnValueStorage, Arguments...);

        // Take the ownership of the return value storage. It has already been constructed for us and placed into the allocated memory.
        return Dyn<TReturnType>( ReturnValueStorage, TakeMemoryOwnership );
    }
};
/// Implementation for dynamic-type-returning non-const member functions
template<typename TReceiverType, typename TReturnType, typename... TFunctionArguments>
struct TMemberVirtualFunctionInvoker<Dyn<TReturnType>(TReceiverType::*)(TFunctionArguments...) const>
{
    static Dyn<TReturnType> Invoke(GenericFunctionPtr FunctionPtr, const TReceiverType* Receiver, TFunctionArguments... Arguments)
    {
        static IDynamicTypeLayout* StaticReturnType = TReturnType::StaticType();
        using InvokeFunctionPtr = TReturnType*(*)(const TReceiverType*, TReturnType*, TFunctionArguments...);

        // Allocate enough memory from the system allocator to hold the return value
        auto* ReturnValueStorage = static_cast<TReturnType*>(malloc(StaticReturnType->GetSize()));

        // Call the function pointer and pass it reference to the memory where return value should be written
        reinterpret_cast<InvokeFunctionPtr>(FunctionPtr)(Receiver, ReturnValueStorage, Arguments...);

        // Take the ownership of the return value storage. It has already been constructed for us and placed into the allocated memory.
        return Dyn<TReturnType>( ReturnValueStorage, TakeMemoryOwnership );
    }
};
