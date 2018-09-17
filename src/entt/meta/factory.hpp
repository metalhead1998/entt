#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <cassert>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../core/hashed_string.hpp"
#include "meta.hpp"


namespace entt {


template<typename>
class meta_factory;


template<typename Type, typename... Property>
meta_factory<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT;


/**
 * @brief A meta factory to be used for reflection purposes.
 *
 * A meta factory is an utility class used to reflect types, data and functions
 * of all sorts. This class ensures that the underlying web of types is built
 * correctly and performs some checks in debug mode to ensure that there are no
 * subtle errors at runtime.
 *
 * @tparam Type Reflected type for which the factory was created.
 */
template<typename Type>
class meta_factory {
    template<auto Data>
    static std::enable_if_t<std::is_member_object_pointer_v<decltype(Data)>, decltype(std::declval<Type>().*Data)>
    actual_type();

    template<auto Data>
    static std::enable_if_t<!std::is_member_object_pointer_v<decltype(Data)>, decltype(*Data)>
    actual_type();

    template<auto Data>
    using data_type = std::remove_reference_t<decltype(meta_factory::actual_type<Data>())>;

    template<auto Func>
    using func_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;

    template<typename Name, typename Node>
    inline bool duplicate(const Name &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline bool duplicate(const meta_any &key, const internal::meta_prop_node *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename>
    internal::meta_prop_node * properties() {
        return nullptr;
    }

    template<typename Owner, typename Property, typename... Other>
    internal::meta_prop_node * properties(Property &&property, Other &&... other) {
        static const meta_any key{std::get<0>(property)};
        static const meta_any value{std::get<1>(property)};

        static internal::meta_prop_node node{
            properties<Owner>(std::forward<Other>(other)...),
            []() -> const meta_any & {
                return key;
            },
            []() -> const meta_any & {
                return value;
            },
            []() {
                static meta_prop meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(key, node.next));
        return &node;
    }

    template<typename... Property>
    meta_factory<Type> clazz(hashed_string name, Property &&... property) ENTT_NOEXCEPT {
        static internal::meta_type_node node{
            name,
            internal::meta_info<>::type,
            properties<Type>(std::forward<Property>(property)...),
            &internal::destroy<Type>,
            []() {
                static meta_type meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(name, node.next));
        assert(!internal::meta_info<Type>::type);
        internal::meta_info<Type>::type = &node;
        internal::meta_info<>::type = &node;

        return *this;
    }

    meta_factory() ENTT_NOEXCEPT = default;

public:
    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of
     * constructors. All that is required is that they return an instance of the
     * underlying type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or a free function.
     *
     * @tparam Func The actual function to use as a constructor.
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename... Property>
    meta_factory & ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;
        static_assert(std::is_same_v<typename helper_type::return_type, Type>);
        auto * const type = internal::meta_info<Type>::type;

        static internal::meta_ctor_node node{
            type->ctor,
            properties<typename helper_type::args_type>(std::forward<Property>(property)...),
            helper_type::size,
            &internal::meta_info<Type>::resolve,
            &helper_type::arg,
            &helper_type::accept,
            [](const meta_any * const any) {
                return internal::invoke<Func>(nullptr, any, std::make_index_sequence<helper_type::size>{});
            },
            []() {
                static meta_ctor meta{&node};
                return &meta;
            }
        };

        assert((!internal::meta_info<Type>::template ctor<typename helper_type::args_type>));
        internal::meta_info<Type>::template ctor<typename helper_type::args_type> = &node;
        type->ctor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * A meta constructor is uniquely identified by the types of its arguments
     * and is such that there exists an actual constructor of the underlying
     * type that can be invoked with parameters whose types are those given.
     *
     * @tparam Args Types of arguments to use to construct an instance.
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<typename... Args, typename... Property>
    meta_factory & ctor(Property &&... property) ENTT_NOEXCEPT {
        using helper_type = internal::meta_function_helper<Type(Args...)>;
        auto * const type = internal::meta_info<Type>::type;

        static internal::meta_ctor_node node{
            type->ctor,
            properties<typename helper_type::args_type>(std::forward<Property>(property)...),
            helper_type::size,
            &internal::meta_info<Type>::resolve,
            &helper_type::arg,
            &helper_type::accept,
            [](const meta_any * const any) {
                return internal::construct<Type, Args...>(any, std::make_index_sequence<helper_type::size>{});
            },
            []() {
                static meta_ctor meta{&node};
                return &meta;
            }
        };

        assert((!internal::meta_info<Type>::template ctor<typename helper_type::args_type>));
        internal::meta_info<Type>::template ctor<typename helper_type::args_type> = &node;
        type->ctor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta destructor to a meta type.
     *
     * Free functions can be assigned to meta types in the role of destructors.
     * The signature of the function should identical to the following:
     *
     * @code{.cpp}
     * void(Type &);
     * @endcode
     *
     * From a client's point of view, nothing changes if the destructor of a
     * meta type is the default one or a custom one.
     *
     * @tparam Func The actual function to use as a destructor.
     * @tparam Property Types of properties to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto *Func, typename... Property>
    std::enable_if_t<std::is_invocable_v<decltype(Func), Type &>, meta_factory &>
    dtor(Property &&... property) ENTT_NOEXCEPT {
        static internal::meta_dtor_node node{
            properties<std::integral_constant<decltype(Func), Func>>(std::forward<Property>(property)...),
            &internal::meta_info<Type>::resolve,
            [](void *instance) {
                (*Func)(*static_cast<Type *>(instance));
            },
            []() {
                static meta_dtor meta{&node};
                return &meta;
            }
        };

        assert(!internal::meta_info<Type>::type->dtor);
        assert((!internal::meta_info<Type>::template dtor<Func>));
        internal::meta_info<Type>::template dtor<Func> = &node;
        internal::meta_info<Type>::type->dtor = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type.
     *
     * Both data members and static or global variables can be assigned to a
     * meta type.<br/>
     * From a client's point of view, all the variables associated with the
     * reflected object will appear as if they were part of the type.
     *
     * @tparam Data The actual variable to attach to the meta type.
     * @tparam Property Types of properties to assign to the meta data.
     * @param str The name to assign to the meta data.
     * @param property Properties to assign to the meta data.
     * @return A meta factory for the parent type.
     */
    template<auto Data, typename... Property>
    meta_factory & data(const char *str, Property &&... property) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::type;

        static internal::meta_data_node node{
            hashed_string{str},
            type->data,
            properties<std::integral_constant<decltype(Data), Data>>(std::forward<Property>(property)...),
            std::is_const_v<data_type<Data>>,
            !std::is_member_object_pointer_v<decltype(Data)>,
            &internal::meta_info<Type>::resolve,
            &internal::meta_info<data_type<Data>>::resolve,
            &internal::setter<std::is_const_v<data_type<Data>>, Type, Data>,
            &internal::getter<Type, Data>,
            [](const internal::meta_type_node * const other) {
                return other == internal::meta_info<data_type<Data>>::resolve();
            },
            []() {
                static meta_data meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(hashed_string{str}, node.next));
        assert((!internal::meta_info<Type>::template data<Data>));
        internal::meta_info<Type>::template data<Data> = &node;
        type->data = &node;

        return *this;
    }

    /**
     * @brief Assigns a meta funcion to a meta type.
     *
     * Both member functions and free functions can be assigned to a meta
     * type.<br/>
     * From a client's point of view, all the functions associated with the
     * reflected object will appear as if they were part of the type.
     *
     * @tparam Func The actual function to attach to the meta type.
     * @tparam Property Types of properties to assign to the meta function.
     * @param str The name to assign to the meta function.
     * @param property Properties to assign to the meta function.
     * @return A meta factory for the parent type.
     */
    template<auto Func, typename... Property>
    meta_factory & func(const char *str, Property &&... property) ENTT_NOEXCEPT {
        auto * const type = internal::meta_info<Type>::type;

        static internal::meta_func_node node{
            hashed_string{str},
            type->func,
            properties<std::integral_constant<decltype(Func), Func>>(std::forward<Property>(property)...),
            func_type<Func>::size,
            func_type<Func>::is_const,
            func_type<Func>::is_static,
            &internal::meta_info<Type>::resolve,
            &internal::meta_info<typename func_type<Func>::return_type>::resolve,
            &func_type<Func>::arg,
            &func_type<Func>::accept,
            [](const void *instance, const meta_any *any) {
                return internal::invoke<Func>(instance, any, std::make_index_sequence<func_type<Func>::size>{});
            },
            [](void *instance, const meta_any *any) {
                return internal::invoke<Func>(instance, any, std::make_index_sequence<func_type<Func>::size>{});
            },
            []() {
                static meta_func meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(hashed_string{str}, node.next));
        assert((!internal::meta_info<Type>::template func<Func>));
        internal::meta_info<Type>::template func<Func> = &node;
        type->func = &node;

        return *this;
    }

    template<typename Other, typename... Property>
    friend meta_factory<Other> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT;
};


/**
 * @brief Basic function to use for reflection.
 *
 * This is the point from which everything starts.<br/>
 * By invoking this function with a type that is not yet reflected, a meta type
 * is created to which it will be possible to attach data and functions through
 * a dedicated factory.
 *
 * @tparam Type Type to reflect.
 * @tparam Property Types of properties to assign to the reflected type.
 * @param str The name to assign to the reflected type.
 * @param property Properties to assign to the reflected type.
 * @return A meta factory for the given type.
 */
template<typename Type, typename... Property>
inline meta_factory<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
    return meta_factory<Type>{}.clazz(hashed_string{str}, std::forward<Property>(property)...);
}


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
inline meta_type * resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve()->meta();
}


/**
 * @brief Returns the meta type associated with a given name.
 * @param str The name to use to search for a meta type.
 * @return The meta type associated with the given name, if any.
 */
inline meta_type * resolve(const char *str) ENTT_NOEXCEPT {
    return internal::meta(hashed_string{str}, internal::meta_info<>::type);
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
void resolve(Op op) ENTT_NOEXCEPT {
    internal::iterate(std::move(op), internal::meta_info<>::type);
}


}


#endif // ENTT_META_FACTORY_HPP
