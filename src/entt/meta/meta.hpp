#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <tuple>
#include <array>
#include <memory>
#include <cstring>
#include <cassert>
#include <cstddef>
#include <utility>
#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


class meta_any;
class meta_prop;
class meta_ctor;
class meta_dtor;
class meta_data;
class meta_func;
class meta_type;


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


struct meta_prop_node;
struct meta_ctor_node;
struct meta_dtor_node;
struct meta_data_node;
struct meta_func_node;
struct meta_type_node;


template<typename...>
struct meta_node {
    static meta_type_node *type;
};


template<typename... Type>
meta_type_node * meta_node<Type...>::type = nullptr;


template<typename Type>
struct meta_node<Type> {
    static meta_type_node *type;

    template<typename>
    static meta_ctor_node *ctor;

    template<auto>
    static meta_dtor_node *dtor;

    template<auto>
    static meta_data_node *data;

    template<auto>
    static meta_func_node *func;

    static internal::meta_type_node * resolve() ENTT_NOEXCEPT;
};


template<typename Type>
meta_type_node * meta_node<Type>::type = nullptr;


template<typename Type>
template<typename>
meta_ctor_node * meta_node<Type>::ctor = nullptr;


template<typename Type>
template<auto>
meta_dtor_node * meta_node<Type>::dtor = nullptr;


template<typename Type>
template<auto>
meta_data_node * meta_node<Type>::data = nullptr;


template<typename Type>
template<auto>
meta_func_node * meta_node<Type>::func = nullptr;


template<typename... Type>
struct meta_info: meta_node<std::decay_t<Type>...> {};


struct meta_prop_node final {
    meta_prop_node * const next;
    const meta_any &(* const key)();
    const meta_any &(* const value)();
    meta_prop *(* const meta)();
};


struct meta_ctor_node final {
    using size_type = std::size_t;
    meta_ctor_node * const next;
    meta_prop_node * const prop;
    const size_type size;
    meta_type_node *(* const parent)();
    meta_type_node *(* const arg)(size_type);
    bool(* const accept)(const meta_type_node ** const);
    meta_any(* const invoke)(const meta_any * const);
    meta_ctor *(* const meta)();
};


struct meta_dtor_node final {
    meta_prop_node * const prop;
    meta_type_node *(* const parent)();
    void(* const invoke)(void *);
    meta_dtor *(* const meta)();
};


struct meta_data_node final {
    const hashed_string name;
    meta_data_node * const next;
    meta_prop_node * const prop;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const parent)();
    meta_type_node *(* const type)();
    void(* const set)(void *, const meta_any &);
    meta_any(* const get)(const void *);
    bool(* const accept)(const meta_type_node * const);
    meta_data *(* const meta)();
};


struct meta_func_node final {
    using size_type = std::size_t;
    const hashed_string name;
    meta_func_node * const next;
    meta_prop_node * const prop;
    const size_type size;
    const bool is_const;
    const bool is_static;
    meta_type_node *(* const parent)();
    meta_type_node *(* const ret)();
    meta_type_node *(* const arg)(size_type);
    bool(* const accept)(const meta_type_node ** const);
    meta_any(* const cinvoke)(const void *, const meta_any *);
    meta_any(* const invoke)(void *, const meta_any *);
    meta_func *(* const meta)();
};


struct meta_type_node final {
    const hashed_string name;
    meta_type_node * const next;
    meta_prop_node * const prop;
    void(* const destroy)(void *);
    meta_type *(* const meta)();
    meta_ctor_node *ctor;
    meta_dtor_node *dtor;
    meta_data_node *data;
    meta_func_node *func;
};


template<typename Op, typename Node>
auto iterate(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr) {
        op(curr->meta());
        curr = curr->next;
    }
}

template<typename Key>
auto property(Key &&key, const meta_prop_node *curr) {
    meta_prop *prop = nullptr;

    iterate([&prop, key = std::forward<Key>(key)](auto *curr) {
        prop = (curr->key().template convertible<Key>() && curr->key() == key) ? curr : prop;
    }, curr);

    return prop;
}

template<typename Node>
auto meta(hashed_string name, const Node *curr) {
    while(curr && curr->name != name) {
        curr = curr->next;
    }

    return curr ? curr->meta() : nullptr;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Meta any object.
 *
 * A meta any is an opaque container for single values of any type.
 *
 * This class uses a technique called small buffer optimization (SBO) to
 * completely eliminate the need to allocate memory, where possible.<br/>
 * From the user's point of view, nothing will change, but the elimination of
 * allocations will reduce the jumps in memory and therefore will avoid chasing
 * of pointers. This will greatly improve the use of the cache, thus increasing
 * the overall performance.
 */
class meta_any final {
    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using compare_fn_type = bool(*)(const void *, const void *);
    using destroy_fn_type = void(*)(void *);

    template<typename Type>
    inline static auto compare(int, const Type &lhs, const Type &rhs)
    -> decltype(lhs == rhs, bool{})
    {
        return lhs == rhs;
    }

    template<typename Type>
    inline static bool compare(char, const Type &, const Type &) {
        return false;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          direct{nullptr},
          destroy{nullptr},
          underlying{nullptr},
          comparator{nullptr}
    {}

    /**
     * @brief Constructs a meta any from a given value.
     *
     * This class uses a technique called small buffer optimization (SBO) to
     * completely eliminate the need to allocate memory, where possible.<br/>
     * From the user's point of view, nothing will change, but the elimination
     * of allocations will reduce the jumps in memory and therefore will avoid
     * chasing of pointers. This will greatly improve the use of the cache, thus
     * increasing the overall performance.
     *
     * @tparam Type Type of object to use to initialize the container.
     * @tparam Embeddable False if an allocation is required, true otherwise.
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type, bool Embeddable = (sizeof(std::decay_t<Type>) <= sizeof(void *))>
    meta_any(Type &&type) {
        using actual_type = std::decay_t<Type>;
        underlying = internal::meta_info<Type>::resolve();

        comparator = [](const void *lhs, const void *rhs) {
            return compare(0, *static_cast<const actual_type *>(lhs), *static_cast<const actual_type *>(rhs));
        };

        if constexpr(Embeddable) {
            new (&storage) actual_type{std::forward<Type>(type)};
            direct = &storage;

            destroy = [](void *direct) {
                static_cast<actual_type *>(direct)->~actual_type();
            };
        } else {
            direct = new actual_type{std::forward<Type>(type)};
            new (&storage) actual_type *{static_cast<actual_type *>(direct)};

            destroy = [](void *direct) {
                delete static_cast<actual_type *>(direct);
            };
        }
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(destroy) {
            destroy(direct);
        }
    }

    /*! @brief Copying a meta any isn't allowed. */
    meta_any(const meta_any &) = delete;

    /**
     * @brief Move constructor.
     *
     * After meta any move construction, instances that have been moved from
     * are placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    meta_any(meta_any &&other) ENTT_NOEXCEPT
        : storage{},
          direct{nullptr},
          destroy{other.destroy},
          underlying{other.underlying},
          comparator{other.comparator}
    {
        std::memcpy(&storage, &other.storage, sizeof(storage_type));
        direct = (other.direct == &other.storage ? &storage : *reinterpret_cast<void **>(&storage));
        other.destroy = nullptr;
    }

    /*! @brief Copying a meta any isn't allowed. @return This container. */
    meta_any & operator=(const meta_any &) = delete;

    /**
     * @brief Move assignment operator.
     *
     * After meta any move assignment, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     * @return This container.
     */
    meta_any & operator=(meta_any &&other) ENTT_NOEXCEPT {
        if(this != &other) {
            if(destroy) {
                destroy(direct);
                destroy = nullptr;
            }

            auto tmp{std::move(other)};
            std::memcpy(&storage, &tmp.storage, sizeof(storage_type));
            direct = (tmp.direct == &tmp.storage ? &storage : *reinterpret_cast<void **>(&storage));
            std::swap(destroy, tmp.destroy);
            std::swap(underlying, tmp.underlying);
            std::swap(comparator, tmp.comparator);
        }

        return *this;
    }

    /**
     * @brief Checks if a meta any contains a properly initialized object.
     * @return True if the underlying object is valid, false otherwise.
     */
    inline bool valid() const ENTT_NOEXCEPT {
        return *this;
    }

    /**
     * @brief Returns the meta type of the underlying object.
     * @return The meta type of the underlying object, if any.
     */
    meta_type * type() const ENTT_NOEXCEPT {
        return underlying ? underlying->meta() : nullptr;
    }

    /**
     * @brief Checks if a meta any is convertible to a given type.
     * @tparam Type Type to which to convert the underlying object.
     * @return True if the underlying object is convertible to the given type,
     * false otherwise.
     */
    template<typename Type>
    bool convertible() const ENTT_NOEXCEPT {
        return internal::meta_info<Type>::resolve() == underlying;
    }

    /**
     * @brief Converts the underlying object to a given type and returns it.
     *
     * The type of the underlying object must coincide exactly with the given
     * one. Otherwise, invoking this function results in an undefined behavior.
     *
     * @tparam Type Type to which to convert the underlying object.
     * @return A reference to the contained object.
     */
    template<typename Type>
    inline const Type & to() const ENTT_NOEXCEPT {
        return *data<Type>();
    }

    /**
     * @brief Converts the underlying object to a given type and returns it.
     *
     * The type of the underlying object must coincide exactly with the given
     * one. Otherwise, invoking this function results in an undefined behavior.
     *
     * @tparam Type Type to which to convert the underlying object.
     * @return A reference to the contained object.
     */
    template<typename Type>
    inline Type & to() ENTT_NOEXCEPT {
        return *data<Type>();
    }

    /**
     * @brief Converts the underlying object to a given type and returns it.
     *
     * The type of the underlying object must coincide exactly with the given
     * one. Otherwise, invoking this function results in an undefined
     * behavior.<br/>
     * In case of an empty container, a null pointer is returned.
     *
     * @tparam Type Type to which to convert the underlying object.
     * @return A pointer to the contained object, if any.
     */
    template<typename Type>
    inline const Type * data() const ENTT_NOEXCEPT {
        return static_cast<const Type *>(data());
    }

    /**
     * @brief Converts the underlying object to a given type and returns it.
     *
     * The type of the underlying object must coincide exactly with the given
     * one. Otherwise, invoking this function results in an undefined
     * behavior.<br/>
     * In case of an empty container, a null pointer is returned.
     *
     * @tparam Type Type to which to convert the underlying object.
     * @return A pointer to the contained object, if any.
     */
    template<typename Type>
    inline Type * data() ENTT_NOEXCEPT {
        return static_cast<Type *>(data());
    }

    /**
     * @brief Returns an opaque pointer to the underlying object.
     * @return An opaque pointer to the contained object, if any.
     */
    inline const void * data() const ENTT_NOEXCEPT {
        return *this;
    }

    /**
     * @brief Returns an opaque pointer to the underlying object.
     * @return An opaque pointer to the contained object, if any.
     */
    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const meta_any *>(this)->data());
    }

    /**
     * @brief Returns an opaque pointer to the underlying object.
     * @return An opaque pointer to the contained object, if any.
     */
    inline operator const void *() const ENTT_NOEXCEPT {
        return direct;
    }

    /**
     * @brief Returns an opaque pointer to the underlying object.
     * @return An opaque pointer to the contained object, if any.
     */
    inline operator void *() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const meta_any *>(this)->data());
    }

    /**
     * @brief Returns false if a container is empty, true otherwise.
     * @return False if the container is empty, true otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return destroy;
    }

    /**
     * @brief Checks if two containers differ in their content.
     * @param other Container with which to compare.
     * @return False if the two containers differ in their content, false
     * otherwise.
     */
    inline bool operator==(const meta_any &other) const ENTT_NOEXCEPT {
        return (!direct && !other.direct) || (direct && other.direct && underlying == other.underlying && comparator(direct, other.direct));
    }

private:
    storage_type storage;
    void *direct;
    destroy_fn_type destroy;
    internal::meta_type_node * underlying;
    compare_fn_type comparator;
};


/**
 * @brief Meta property object.
 *
 * A meta property is an opaque container for a key/value pair.<br/>
 * Properties are associated with any other meta object to enrich it.
 */
class meta_prop {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_prop(internal::meta_prop_node * node)
        : node{node}
    {}

public:
    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    inline const meta_any & key() const ENTT_NOEXCEPT {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    inline const meta_any & value() const ENTT_NOEXCEPT {
        return node->value();
    }

private:
    internal::meta_prop_node *node;
};


/**
 * @brief Meta constructor object.
 *
 * A meta constructor is an opaque container for a function to be used to
 * construct instances of a given type.
 */
class meta_ctor {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_ctor(internal::meta_ctor_node * node)
        : node{node}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /**
     * @brief Returns the number of arguments accepted by a meta constructor.
     * @return The number of arguments accepted by the meta constructor.
     */
    inline size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Returns the meta type to which a meta constructor belongs.
     * @return The meta type to which the meta constructor belongs.
     */
    inline meta_type * parent() const ENTT_NOEXCEPT {
        return node->parent()->meta();
    }

    /**
     * @brief Returns the meta type of the i-th argument of a meta constructor.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta constructor, if any.
     */
    inline meta_type * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node->arg(index)->meta() : nullptr;
    }

    /**
     * @brief Indicates whether a given list of types of arguments is accepted.
     * @tparam Args List of types of arguments to check.
     * @return True if the meta constructor accepts the arguments, false
     * otherwise.
     */
    template<typename... Args>
    bool accept() const ENTT_NOEXCEPT {
        std::array<const internal::meta_type_node *, sizeof...(Args)> args{{internal::meta_info<Args>::resolve()...}};
        return sizeof...(Args) == size() ? node->accept(args.data()) : false;
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the types of the parameters must coincide
     * exactly with those required by the underlying meta constructor.
     * Otherwise, an empty and then invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any invoke(Args &&... args) {
        std::array<const meta_any, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(any.data()) : meta_any{};
    }

    /**
     * @brief Iterates all the properties assigned to a meta constructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop *>, meta_prop *>
    prop(Key &&key) const ENTT_NOEXCEPT {
        return internal::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::meta_ctor_node *node;
};


/**
 * @brief Meta destructor object.
 *
 * A meta destructor is an opaque container for a function to be used to
 * destroy instances of a given type.
 */
class meta_dtor {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_dtor(internal::meta_dtor_node * node)
        : node{node}
    {}

public:
    /**
     * @brief Returns the meta type to which a meta destructor belongs.
     * @return The meta type to which the meta destructor belongs.
     */
    inline meta_type * parent() const ENTT_NOEXCEPT {
        return node->parent()->meta();
    }

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * The actual type of the instance must coincide exactly with that of the
     * parent type of the meta destructor. Otherwise, invoking the meta
     * destructor results in an undefined behavior.
     *
     * @param instance An opaque pointer to an instance of the underlying type.
     */
    inline void invoke(void *instance) {
        node->invoke(instance);
    }

    /**
     * @brief Iterates all the properties assigned to a meta destructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop *>, meta_prop *>
    prop(Key &&key) const ENTT_NOEXCEPT {
        return internal::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::meta_dtor_node *node;
};


/**
 * @brief Meta data object.
 *
 * A meta data is an opaque container for a data member associated with a given
 * type.
 */
class meta_data {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_data(internal::meta_data_node * node)
        : node{node}
    {}

public:
    /**
     * @brief Returns the name assigned to a given meta data.
     * @return The name assigned to the meta data.
     */
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    /**
     * @brief Indicates whether a given meta data is constant or not.
     * @return True if the meta data is constant, false otherwise.
     */
    inline bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta data is static or not.
     *
     * A static meta data is such that it can be accessed using a null pointer
     * as an instance.
     *
     * @return True if the meta data is static, false otherwise.
     */
    inline bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type to which a meta data belongs.
     * @return The meta type to which the meta data belongs.
     */
    inline meta_type * parent() const ENTT_NOEXCEPT {
        return node->parent()->meta();
    }

    /**
     * @brief Returns the meta type of a given meta data.
     * @return The meta type of the meta data.
     */
    inline meta_type * type() const ENTT_NOEXCEPT {
        return node->type()->meta();
    }

    /**
     * @brief Indicates whether a given type of value is accepted.
     * @tparam Type Type of value to check.
     * @return True if the meta data accepts the type of value, false otherwise.
     */
    template<typename Type>
    bool accept() const ENTT_NOEXCEPT {
        return node->accept(internal::meta_info<Type>::resolve());
    }

    /**
     * @brief Sets the value of the variable enclosed by a given meta type.
     *
     * The actual type of the instance must coincide exactly with that of the
     * parent type of the meta data. Otherwise, invoking the setter results in
     * an undefined behavior.<br/>
     * The type of the value must coincide exactly with that of the variable
     * enclosed by the meta data. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param instance An opaque pointer to an instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     */
    template<typename Type>
    void set(void *instance, Type &&value) {
        return accept<Type>() ? node->set(instance, meta_any{std::forward<Type>(value)}) : void();
    }

    /**
     * @brief Gets the value of the variable enclosed by a given meta type.
     *
     * The actual type of the instance must coincide exactly with that of the
     * parent type of the meta function. Otherwise, invoking the getter results
     * in an undefined behavior.
     *
     * @param instance An opaque pointer to an instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    inline meta_any get(const void *instance) const ENTT_NOEXCEPT {
        return node->get(instance);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop *>, meta_prop *>
    prop(Key &&key) const ENTT_NOEXCEPT {
        return internal::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::meta_data_node *node;
};


/**
 * @brief Meta function object.
 *
 * A meta function is an opaque container for a member function associated with
 * a given type.
 */
class meta_func {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_func(internal::meta_func_node * node)
        : node{node}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_ctor_node::size_type;

    /**
     * @brief Returns the name assigned to a given meta function.
     * @return The name assigned to the meta function.
     */
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    /**
     * @brief Returns the number of arguments accepted by a meta function.
     * @return The number of arguments accepted by the meta function.
     */
    inline size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    /**
     * @brief Indicates whether a given meta function is constant or not.
     * @return True if the meta function is constant, false otherwise.
     */
    inline bool is_const() const ENTT_NOEXCEPT {
        return node->is_const;
    }

    /**
     * @brief Indicates whether a given meta function is static or not.
     *
     * A static meta function is such that it can be invoked using a null
     * pointer as an instance.
     *
     * @return True if the meta function is static, false otherwise.
     */
    inline bool is_static() const ENTT_NOEXCEPT {
        return node->is_static;
    }

    /**
     * @brief Returns the meta type to which a meta function belongs.
     * @return The meta type to which the meta function belongs.
     */
    inline meta_type * parent() const ENTT_NOEXCEPT {
        return node->parent()->meta();
    }

    /**
     * @brief Returns the meta type of the return type of a meta function.
     * @return The meta type of the return type of the meta function.
     */
    inline meta_type * ret() const ENTT_NOEXCEPT {
        return node->ret()->meta();
    }

    /**
     * @brief Returns the meta type of the i-th argument of a meta function.
     * @param index The index of the argument of which to return the meta type.
     * @return The meta type of the i-th argument of a meta function, if any.
     */
    inline meta_type * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node->arg(index)->meta() : nullptr;
    }

    /**
     * @brief Indicates whether a given list of types of arguments is accepted.
     * @tparam Args List of types of arguments to check.
     * @return True if the meta function accepts the arguments, false otherwise.
     */
    template<typename... Args>
    bool accept() const ENTT_NOEXCEPT {
        std::array<const internal::meta_type_node *, sizeof...(Args)> args{{internal::meta_info<Args>::resolve()...}};
        return sizeof...(Args) == size() ? node->accept(args.data()) : false;
    }

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a meta function, the types of the parameters must coincide
     * exactly with those required by the underlying function. Otherwise, an
     * empty and then invalid container is returned.<br/>
     * The actual type of the instance must coincide exactly with that of the
     * parent type of the meta function. Otherwise, invoking the underlying
     * function results in an undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param instance An opaque pointer to an instance of the underlying type.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(const void *instance, Args &&... args) const {
        std::array<const meta_any, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->cinvoke(instance, any.data()) : meta_any{};
    }

    /**
     * @brief Invokes the underlying function, if possible.
     *
     * To invoke a meta function, the types of the parameters must coincide
     * exactly with those required by the underlying function. Otherwise, an
     * empty and then invalid container is returned.<br/>
     * The actual type of the instance must coincide exactly with that of the
     * parent type of the meta function. Otherwise, invoking the underlying
     * function results in an undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param instance An opaque pointer to an instance of the underlying type.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(void *instance, Args &&... args) {
        std::array<const meta_any, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(instance, any.data()) : meta_any{};
    }

    /**
     * @brief Iterates all the properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop *>, meta_prop *>
    prop(Key &&key) const ENTT_NOEXCEPT {
        return internal::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::meta_func_node *node;
};


/**
 * @brief Meta type object.
 *
 * A meta type is the starting point for accessing a reflected type, thus being
 * able to work through it on real objects.
 */
class meta_type {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_type(internal::meta_type_node * node) ENTT_NOEXCEPT
        : node{node}
    {}

public:
    /**
     * @brief Returns the name assigned to a given meta type.
     * @return The name assigned to the meta type.
     */
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    /**
     * @brief Iterates all the meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void ctor(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->ctor);
    }

    /**
     * @brief Returns the meta constructor that accepts a given list of types of
     * arguments.
     * @return The requested meta constructor, if any.
     */
    template<typename... Args>
    inline meta_ctor * ctor() const ENTT_NOEXCEPT {
        meta_ctor *meta = nullptr;

        ctor([&meta](meta_ctor *curr) {
            meta = curr->accept<Args...>() ? curr : meta;
        });

        return meta;
    }

    /**
     * @brief Iterates all the meta destructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void dtor(Op op) const ENTT_NOEXCEPT {
        return node->dtor ? op(node->dtor->meta()) : void();
    }

    /**
     * @brief Returns the meta destructor associated with a given type.
     * @return The meta destructor associated with the given type, if any.
     */
    inline meta_dtor * dtor() const ENTT_NOEXCEPT {
        return node->dtor ? node->dtor->meta() : nullptr;
    }

    /**
     * @brief Iterates all the meta data of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void data(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->data);
    }

    /**
     * @brief Returns the meta data associated with a given name.
     * @param str The name to use to search for a meta data.
     * @return The meta data associated with the given name, if any.
     */
    inline meta_data * data(const char *str) const ENTT_NOEXCEPT {
        return internal::meta(hashed_string{str}, node->data);
    }

    /**
     * @brief Iterates all the meta functions of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void func(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->func);
    }

    /**
     * @brief Returns the meta function associated with a given name.
     * @param str The name to use to search for a meta function.
     * @return The meta function associated with the given name, if any.
     */
    inline meta_func * func(const char *str) const ENTT_NOEXCEPT {
        return internal::meta(hashed_string{str}, node->func);
    }

    /**
     * @brief Creates an instance of the underlying type, if possible.
     *
     * To create a valid instance, the types of the parameters must coincide
     * exactly with those required by the underlying meta constructor.
     * Otherwise, an empty and then invalid container is returned.
     *
     * @tparam Args Types of arguments to use to construct the instance.
     * @param args Parameters to use to construct the instance.
     * @return A meta any containing the new instance, if any.
     */
    template<typename... Args>
    meta_any construct(Args &&... args) const {
        auto *curr = node->ctor;

        while(curr && !curr->meta()->template accept<Args...>()) {
            curr = curr->next;
        }

        return curr ? curr->meta()->invoke(std::forward<Args>(args)...) : meta_any{};
    }

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * The actual type of the instance must coincide exactly with that of the
     * meta type. Otherwise, invoking the meta destructor results in an
     * undefined behavior.
     *
     * @param instance An opaque pointer to an instance of the underlying type.
     */
    inline void destroy(void *instance) {
        return node->dtor ? node->dtor->invoke(instance) : node->destroy(instance);
    }

    /**
     * @brief Iterates all the properties assigned to a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate(std::move(op), node->prop);
    }

    /**
     * @brief Returns the property associated with a given key.
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop *>, meta_prop *>
    prop(Key &&key) const ENTT_NOEXCEPT {
        return internal::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::meta_type_node *node;
};


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename...>
struct meta_function_helper;


template<typename Ret, typename... Args>
struct meta_function_helper<Ret(Args...)> {
    using return_type = Ret;
    using args_type = std::tuple<Args...>;
    static constexpr auto size = sizeof...(Args);

    static auto arg(typename internal::meta_func_node::size_type index) {
        return std::array<internal::meta_type_node *, sizeof...(Args)>{{internal::meta_info<Args>::resolve()...}}[index];
    }

    static auto accept(const internal::meta_type_node ** const types) {
        std::array<internal::meta_type_node *, sizeof...(Args)> args{{internal::meta_info<Args>::resolve()...}};
        return std::equal(args.cbegin(), args.cend(), types);
    }
};


template<typename Class, typename Ret, typename... Args, bool Const, bool Static>
struct meta_function_helper<Class, Ret(Args...), std::bool_constant<Const>, std::bool_constant<Static>>: meta_function_helper<Ret(Args...)> {
    using class_type = Class;
    static constexpr auto is_const = Const;
    static constexpr auto is_static = Static;
};


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Class, Ret(Args...), std::bool_constant<false>, std::bool_constant<false>>
to_meta_function_helper(Ret(Class:: *)(Args...));


template<typename Ret, typename... Args, typename Class>
constexpr meta_function_helper<Class, Ret(Args...), std::bool_constant<true>, std::bool_constant<false>>
to_meta_function_helper(Ret(Class:: *)(Args...) const);


template<typename Ret, typename... Args>
constexpr meta_function_helper<void, Ret(Args...), std::bool_constant<false>, std::bool_constant<true>>
to_meta_function_helper(Ret(*)(Args...));


template<auto Func>
struct meta_function_helper<std::integral_constant<decltype(Func), Func>>: decltype(to_meta_function_helper(Func)) {};


template<typename Type>
void destroy([[maybe_unused]] void *instance) {
    if constexpr(std::is_void_v<Type>) {
        assert(false);
    } else {
        static_cast<Type *>(instance)->~Type();
    }
}


template<typename Type, typename... Args, std::size_t... Indexes>
meta_any construct(const meta_any * const any, std::index_sequence<Indexes...>) {
    return meta_any{Type{(any+Indexes)->to<std::decay_t<Args>>()...}};
}


template<bool Const, typename Type, auto Data>
void setter([[maybe_unused]] void *instance, [[maybe_unused]] const meta_any &any) {
    if constexpr(Const) {
        assert(false);
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        static_cast<Type *>(instance)->*Data = any.to<std::decay_t<decltype(std::declval<Type>().*Data)>>();
    } else {
        *Data = any.to<std::decay_t<decltype(*Data)>>();
    }
}


template<typename Type, auto Data>
meta_any getter([[maybe_unused]] const void *instance) {
    if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        return meta_any{static_cast<const Type *>(instance)->*Data};
    } else {
        return meta_any{*Data};
    }
}


template<auto Func, std::size_t... Indexes>
std::enable_if_t<std::is_function_v<std::remove_pointer_t<decltype(Func)>>, meta_any>
invoke(const void *, const meta_any *any, std::index_sequence<Indexes...>) {
    using helper_type = internal::meta_function_helper<std::integral_constant<decltype(Func), Func>>;
    meta_any result{};

    if constexpr(std::is_void_v<typename helper_type::return_type>) {
        (*Func)((any+Indexes)->to<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...);
    } else {
        result = meta_any{(*Func)((any+Indexes)->to<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...)};
    }

    return result;
}


template<auto Member, std::size_t... Indexes>
std::enable_if_t<std::is_member_function_pointer_v<decltype(Member)>, meta_any>
invoke([[maybe_unused]] const void *instance, [[maybe_unused]] const meta_any *any, std::index_sequence<Indexes...>) {
    using helper_type = internal::meta_function_helper<std::integral_constant<decltype(Member), Member>>;
    meta_any result{};

    if constexpr(helper_type::is_const) {
        const auto *clazz = static_cast<const typename helper_type::class_type *>(instance);

        if constexpr(std::is_void_v<typename helper_type::return_type>) {
            (clazz->*Member)((any+Indexes)->to<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...);
        } else {
            result = meta_any{(clazz->*Member)((any+Indexes)->to<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...)};
        }
    } else {
        assert(false);
    }

    return result;
}


template<auto Member, std::size_t... Indexes>
std::enable_if_t<std::is_member_function_pointer_v<decltype(Member)>, meta_any>
invoke(void *instance, const meta_any *any, std::index_sequence<Indexes...>) {
    using helper_type = internal::meta_function_helper<std::integral_constant<decltype(Member), Member>>;
    meta_any result{};

    if constexpr(helper_type::is_const) {
        result = invoke<Member>(static_cast<const void *>(instance), any, std::make_index_sequence<sizeof...(Indexes)>{});
    } else {
        auto *clazz = static_cast<typename helper_type::class_type *>(instance);

        if constexpr(std::is_void_v<typename helper_type::return_type>) {
            (clazz->*Member)((any+Indexes)->to<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...);
        } else {
            result = meta_any{(clazz->*Member)((any+Indexes)->to<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...)};
        }
    }

    return result;
}


template<typename Type>
meta_type_node * meta_node<Type>::resolve() ENTT_NOEXCEPT {
    if(!type) {
        static meta_type_node node{
            {},
            nullptr,
            nullptr,
            &destroy<Type>,
            []() -> meta_type * {
                return nullptr;
            }
        };

        type = &node;
    }

    return type;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


}


#endif // ENTT_META_META_HPP
