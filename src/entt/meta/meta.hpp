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


struct meta_handle;
class meta_any;
class meta_prop;
class meta_base;
class meta_conv;
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
struct meta_base_node;
struct meta_conv_node;
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
    static meta_base_node *base;

    template<typename>
    static meta_conv_node *conv;

    template<typename>
    static meta_ctor_node *ctor;

    template<auto>
    static meta_dtor_node *dtor;

    template<auto>
    static meta_data_node *data;

    template<auto>
    static meta_func_node *func;

    static meta_type_node * resolve() ENTT_NOEXCEPT;
};


template<typename Type>
meta_type_node * meta_node<Type>::type = nullptr;


template<typename Type>
template<typename>
meta_base_node * meta_node<Type>::base = nullptr;


template<typename Type>
template<typename>
meta_conv_node * meta_node<Type>::conv = nullptr;


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
    meta_any(* const key)();
    meta_any(* const value)();
    meta_prop *(* const meta)();
};


struct meta_base_node final {
    meta_base_node * const next;
    meta_type_node *(* const parent)();
    meta_type_node *(* const type)();
    void *(* const cast)(void *);
    meta_base *(* const meta)();
};


struct meta_conv_node final {
    meta_conv_node * const next;
    meta_type_node *(* const parent)();
    meta_type_node *(* const type)();
    meta_any(* const conv)(void *);
    meta_conv *(* const meta)();
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
    void(* const invoke)(meta_handle);
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
    void(* const set)(meta_handle, const meta_any &);
    meta_any(* const get)(meta_handle);
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
    meta_any(* const invoke)(meta_handle, const meta_any *);
    meta_func *(* const meta)();
};


struct meta_type_node final {
    const hashed_string name;
    meta_type_node * const next;
    meta_prop_node * const prop;
    meta_handle(* const handle)(void *);
    void(* const destroy)(meta_handle);
    meta_type *(* const meta)();
    meta_base_node *base;
    meta_conv_node *conv;
    meta_ctor_node *ctor;
    meta_dtor_node *dtor;
    meta_data_node *data;
    meta_func_node *func;
};


template<typename Op, typename Node>
void iterate(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr) {
        op(curr);
        curr = curr->next;
    }
}


template<auto Member, typename Op>
void iterate(Op op, const meta_type_node *node) ENTT_NOEXCEPT {
    if(node) {
        auto *curr = node->base;
        iterate(op, node->*Member);

        while(curr) {
            iterate<Member>(op, curr->type());
            curr = curr->next;
        }
    }
}


template<typename Op, typename Node>
auto find_if(Op op, const Node *curr) ENTT_NOEXCEPT {
    while(curr && !op(curr)) {
        curr = curr->next;
    }

    return curr ? curr->meta() : nullptr;
}


template<auto Member, typename Op>
auto find_if(Op op, const meta_type_node *node) ENTT_NOEXCEPT
-> decltype(find_if(op, node->*Member))
{
    decltype(find_if(op, node->*Member)) ret = nullptr;

    if(node) {
        ret = find_if(op, node->*Member);
        auto *curr = node->base;

        while(curr && !ret) {
            ret = find_if<Member>(op, curr->type());
            curr = curr->next;
        }
    }

    return ret;
}


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Meta handle object.
 *
 * A meta handle is an opaque pointer to an instance of any type.
 *
 * A handle doesn't perform copies and isn't responsible for the contained
 * object. It doesn't prolong the lifetime of the pointed instance. Users are
 * responsible for ensuring that the target object remains alive for the entire
 * interval of use of the handle.
 */
struct meta_handle final {
    /*! @brief Default constructor. */
    meta_handle() ENTT_NOEXCEPT
        : node{nullptr},
          instance{nullptr}
    {}

    /**
     * @brief Constructs a meta handle from a given instance.
     * @tparam Type Type of object to use to initialize the handle.
     * @param instance A reference to an object to use to initialize the handle.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(Type &&instance) ENTT_NOEXCEPT
        : node{internal::meta_info<Type>::resolve()},
          instance{&instance}
    {}

    /*! @brief Default destructor. */
    ~meta_handle() ENTT_NOEXCEPT = default;

    /*! @brief Default copy constructor. */
    meta_handle(const meta_handle &) ENTT_NOEXCEPT = default;

    /*! @brief Default move constructor. */
    meta_handle(meta_handle &&) ENTT_NOEXCEPT = default;

    /*! @brief Default copy assignment operator. @return This handle. */
    meta_handle & operator=(const meta_handle &) ENTT_NOEXCEPT = default;

    /*! @brief Default move assignment operator. @return This handle. */
    meta_handle & operator=(meta_handle &&) ENTT_NOEXCEPT = default;

    /**
     * @brief Returns the meta type of the underlying object.
     * @return The meta type of the underlying object, if any.
     */
    inline meta_type * type() const ENTT_NOEXCEPT {
        return node ? node->meta() : nullptr;
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the conversion is possible.
     *
     * @warning
     * Attempting to perform a conversion that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the conversion is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A pointer to the contained instance.
     */
    template<typename Type>
    inline const Type * try_cast() const ENTT_NOEXCEPT {
        const auto *type = internal::meta_info<Type>::resolve();
        void *ret = nullptr;

        if(node == type) {
            ret = instance;
        } else {
            const auto *base = internal::find_if<&internal::meta_type_node::base>([type](auto *node) {
                return node->type() == type;
            }, node);

            ret = base ? base->cast(instance) : nullptr;
        }

        return static_cast<const Type *>(ret);
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the conversion is possible.
     *
     * @warning
     * Attempting to perform a conversion that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the conversion is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A pointer to the contained instance.
     */
    template<typename Type>
    inline Type * try_cast() ENTT_NOEXCEPT {
        return const_cast<Type *>(const_cast<const meta_handle *>(this)->try_cast<Type>());
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    inline const void * data() const ENTT_NOEXCEPT {
        return instance;
    }

    /**
     * @brief Returns an opaque pointer to the contained instance.
     * @return An opaque pointer the contained instance, if any.
     */
    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const meta_handle *>(this)->data());
    }

    /**
     * @brief Returns false if a handle is empty, true otherwise.
     * @return False if the handle is empty, true otherwise.
     */
    inline explicit operator bool() const ENTT_NOEXCEPT {
        return instance;
    }

private:
    const internal::meta_type_node *node;
    void *instance;
};


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
    inline static bool compare(char, const Type &lhs, const Type &rhs) {
        return &lhs == &rhs;
    }

public:
    /*! @brief Default constructor. */
    meta_any() ENTT_NOEXCEPT
        : storage{},
          instance{nullptr},
          destroy{nullptr},
          node{nullptr},
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
     * @param type An instance of an object to use to initialize the container.
     */
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(Type &&type) {
        using actual_type = std::decay_t<Type>;
        node = internal::meta_info<Type>::resolve();

        comparator = [](const void *lhs, const void *rhs) {
            return compare(0, *static_cast<const actual_type *>(lhs), *static_cast<const actual_type *>(rhs));
        };

        if constexpr(sizeof(std::decay_t<Type>) <= sizeof(void *)) {
            new (&storage) actual_type{std::forward<Type>(type)};
            instance = &storage;

            destroy = [](void *instance) {
                static_cast<actual_type *>(instance)->~actual_type();
            };
        } else {
            instance = new actual_type{std::forward<Type>(type)};
            new (&storage) actual_type *{static_cast<actual_type *>(instance)};

            destroy = [](void *instance) {
                delete static_cast<actual_type *>(instance);
            };
        }
    }

    /*! @brief Frees the internal storage, whatever it means. */
    ~meta_any() {
        if(destroy) {
            destroy(instance);
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
          instance{nullptr},
          destroy{other.destroy},
          node{other.node},
          comparator{other.comparator}
    {
        std::memcpy(&storage, &other.storage, sizeof(storage_type));
        instance = (other.instance == &other.storage ? &storage : *reinterpret_cast<void **>(&storage));
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
                destroy(instance);
                destroy = nullptr;
            }

            auto tmp{std::move(other)};
            std::memcpy(&storage, &tmp.storage, sizeof(storage_type));
            instance = (tmp.instance == &tmp.storage ? &storage : *reinterpret_cast<void **>(&storage));
            std::swap(destroy, tmp.destroy);
            std::swap(node, tmp.node);
            std::swap(comparator, tmp.comparator);
        }

        return *this;
    }

    /**
     * @brief Returns the meta type of the underlying object.
     * @return The meta type of the underlying object, if any.
     */
    inline meta_type * type() const ENTT_NOEXCEPT {
        return node ? node->meta() : nullptr;
    }

    /**
     * @brief Returns a handle for the underlying instance of a meta any.
     * @return A handle for the underlying instance of a meta any, if any.
     */
    inline meta_handle handle() const ENTT_NOEXCEPT {
        return node ? node->handle(instance) : meta_handle{};
    }

    /**
     * @brief Checks if it's possible to cast an instance to a given type.
     * @tparam Type Type to which to cast the instance.
     * @return True if the conversion is viable, false otherwise.
     */
    template<typename Type>
    inline bool can_cast() const ENTT_NOEXCEPT {
        const auto *type = internal::meta_info<Type>::resolve();

        return (node == type) || internal::find_if<&internal::meta_type_node::base>([type](auto *node) {
            return node->type() == type;
        }, node);
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the conversion is possible.
     *
     * @warning
     * Attempting to perform a conversion that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the conversion is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    inline const Type & cast() const ENTT_NOEXCEPT {
        assert(can_cast<Type>());
        return *handle().try_cast<Type>();
    }

    /**
     * @brief Tries to cast an instance to a given type.
     *
     * The type of the instance must be such that the conversion is possible.
     *
     * @warning
     * Attempting to perform a conversion that isn't viable results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the conversion is not feasible.
     *
     * @tparam Type Type to which to cast the instance.
     * @return A reference to the contained instance.
     */
    template<typename Type>
    inline Type & cast() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const meta_any *>(this)->cast<Type>());
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Type TODO
     * @return TODO
     */
    template<typename Type>
    inline bool can_convert() const ENTT_NOEXCEPT {
        const auto *type = internal::meta_info<Type>::resolve();

        return (node == type) || internal::find_if<&internal::meta_type_node::conv>([type](auto *node) {
            return node->type() == type;
        }, node);
    }

    /**
     * @brief TODO
     *
     * TODO
     *
     * @tparam Type TODO
     * @return TODO
     */
    template<typename Type>
    inline meta_any convert() const ENTT_NOEXCEPT {
        assert(can_convert<Type>());
        const auto *type = internal::meta_info<Type>::resolve();
        meta_any any{};

        if(node == type) {
            any = *static_cast<const Type *>(instance);
        } else {
            const auto *conv = internal::find_if<&internal::meta_type_node::conv>([type](auto *node) {
                return node->type() == type;
            }, node);

            if(conv) {
                any = conv->conv(instance);
            }
        }

        return any;
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
     * @return False if the two containers differ in their content, true
     * otherwise.
     */
    inline bool operator==(const meta_any &other) const ENTT_NOEXCEPT {
        return (!instance && !other.instance) || (instance && other.instance && node == other.node && comparator(instance, other.instance));
    }

private:
    storage_type storage;
    void *instance;
    destroy_fn_type destroy;
    internal::meta_type_node *node;
    compare_fn_type comparator;
};


/**
 * @brief Checks if two containers differ in their content.
 * @param lhs A meta any object, either empty or not.
 * @param rhs A meta any object, either empty or not.
 * @return True if the two containers differ in their content, false otherwise.
 */
inline bool operator!=(const meta_any &lhs, const meta_any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Meta property object.
 *
 * A meta property is an opaque container for a key/value pair.<br/>
 * Properties are associated with any other meta object to enrich it.
 */
class meta_prop final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_prop(internal::meta_prop_node * node) ENTT_NOEXCEPT
        : node{node}
    {}

public:
    /**
     * @brief Returns the stored key.
     * @return A meta any containing the key stored with the given property.
     */
    inline meta_any key() const ENTT_NOEXCEPT {
        return node->key();
    }

    /**
     * @brief Returns the stored value.
     * @return A meta any containing the value stored with the given property.
     */
    inline meta_any value() const ENTT_NOEXCEPT {
        return node->value();
    }

private:
    internal::meta_prop_node *node;
};


/**
 * @brief Meta base object.
 *
 * A meta base is an opaque container for a base class to be used to walk
 * through hierarchies.
 */
class meta_base final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_base(internal::meta_base_node * node) ENTT_NOEXCEPT
        : node{node}
    {}

public:
    /**
     * @brief Returns the meta type to which a meta base belongs.
     * @return The meta type to which the meta base belongs.
     */
    inline meta_type * parent() const ENTT_NOEXCEPT {
        return node->parent()->meta();
    }

    /**
     * @brief Returns the meta type of a given meta base.
     * @return The meta type of the meta base.
     */
    inline meta_type * type() const ENTT_NOEXCEPT {
        return node->type()->meta();
    }

    /**
     * @brief Casts an instance from a parent type to a base type.
     * @param instance The instance to cast.
     * @return An opaque pointer to the base type.
     */
    inline void * cast(void *instance) const ENTT_NOEXCEPT {
        return node->cast(instance);
    }

private:
    internal::meta_base_node *node;
};


/**
 * @brief Meta conversion function object.
 *
 * A meta conversion function is an opaque container for a conversion function
 * to be used to convert a given instance to another type.
 */
class meta_conv final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_conv(internal::meta_conv_node * node) ENTT_NOEXCEPT
        : node{node}
    {}

public:
    /**
     * @brief Returns the meta type to which a meta conversion function belongs.
     * @return The meta type to which the meta conversion function belongs.
     */
    inline meta_type * parent() const ENTT_NOEXCEPT {
        return node->parent()->meta();
    }

    /**
     * @brief Returns the meta type of a given meta conversion function.
     * @return The meta type of the meta conversion function.
     */
    inline meta_type * type() const ENTT_NOEXCEPT {
        return node->type()->meta();
    }

    /**
     * @brief Converts an instance to a given type.
     * @param instance The instance to convert.
     * @return An opaque pointer to the instance to convert.
     */
    inline meta_any convert(void *instance) const ENTT_NOEXCEPT {
        return node->conv(instance);
    }

private:
    internal::meta_conv_node *node;
};


/**
 * @brief Meta constructor object.
 *
 * A meta constructor is an opaque container for a function to be used to
 * construct instances of a given type.
 */
class meta_ctor final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_ctor(internal::meta_ctor_node * node) ENTT_NOEXCEPT
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
    meta_any invoke(Args &&... args) const {
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
        internal::iterate([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node->prop);
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
        return internal::find_if([key = std::forward<Key>(key)](auto *curr) {
            const auto &id = curr->meta()->key();
            return id.template can_cast<Key>() && id == key;
        }, node->prop);
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
class meta_dtor final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_dtor(internal::meta_dtor_node * node) ENTT_NOEXCEPT
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
     * It must be possible to cast the instance to the parent type of the meta
     * destructor. Otherwise, invoking the meta destructor results in an
     * undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     */
    inline void invoke(meta_handle handle) const {
        node->invoke(handle);
    }

    /**
     * @brief Iterates all the properties assigned to a meta destructor.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node->prop);
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
        return internal::find_if([key = std::forward<Key>(key)](auto *curr) {
            const auto &id = curr->meta()->key();
            return id.template can_cast<Key>() && id == key;
        }, node->prop);
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
class meta_data final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_data(internal::meta_data_node * node) ENTT_NOEXCEPT
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
    inline bool accept() const ENTT_NOEXCEPT {
        return node->accept(internal::meta_info<Type>::resolve());
    }

    /**
     * @brief Sets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * data. Otherwise, invoking the setter results in an undefined
     * behavior.<br/>
     * The type of the value must coincide exactly with that of the variable
     * enclosed by the meta data. Otherwise, invoking the setter does nothing.
     *
     * @tparam Type Type of value to assign.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param value Parameter to use to set the underlying variable.
     */
    template<typename Type>
    inline void set(meta_handle handle, Type &&value) const {
        return accept<Type>() ? node->set(handle, meta_any{std::forward<Type>(value)}) : void();
    }

    /**
     * @brief Gets the value of the variable enclosed by a given meta type.
     *
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the getter results in an undefined
     * behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     * @return A meta any containing the value of the underlying variable.
     */
    inline meta_any get(meta_handle handle) const ENTT_NOEXCEPT {
        return node->get(handle);
    }

    /**
     * @brief Iterates all the properties assigned to a meta data.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node->prop);
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
        return internal::find_if([key = std::forward<Key>(key)](auto *curr) {
            const auto &id = curr->meta()->key();
            return id.template can_cast<Key>() && id == key;
        }, node->prop);
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
class meta_func final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    meta_func(internal::meta_func_node * node) ENTT_NOEXCEPT
        : node{node}
    {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename internal::meta_func_node::size_type;

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
     * It must be possible to cast the instance to the parent type of the meta
     * function. Otherwise, invoking the underlying function results in an
     * undefined behavior.
     *
     * @tparam Args Types of arguments to use to invoke the function.
     * @param handle An opaque pointer to an instance of the underlying type.
     * @param args Parameters to use to invoke the function.
     * @return A meta any containing the returned value, if any.
     */
    template<typename... Args>
    meta_any invoke(meta_handle handle, Args &&... args) const {
        std::array<const meta_any, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(std::move(handle), any.data()) : meta_any{};
    }

    /**
     * @brief Iterates all the properties assigned to a meta function.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node->prop);
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
        return internal::find_if([key = std::forward<Key>(key)](auto *curr) {
            const auto &id = curr->meta()->key();
            return id.template can_cast<Key>() && id == key;
        }, node->prop);
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
class meta_type final {
    /*! @brief A factory is allowed to create meta objects. */
    template<typename>
    friend class meta_factory;

    /*! @brief A meta node is allowed to create meta objects. */
    template<typename...>
    friend struct internal::meta_node;

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
     * @brief Iterates all the meta base of a meta type.
     *
     * Iteratively returns **all** the base classes of the given type.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void base(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::base>([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node);
    }

    /**
     * @brief Returns the meta base associated with a given name.
     *
     * Searches recursively among **all** the base classes of the given type.
     *
     * @param str The name to use to search for a meta base.
     * @return The meta base associated with the given name, if any.
     */
    inline meta_base * base(const char *str) const ENTT_NOEXCEPT {
        return internal::find_if<&internal::meta_type_node::base>([name = hashed_string{str}](auto *node) {
            return node->type()->name == name;
        }, node);
    }

    /**
     * @brief Iterates all the meta constructors of a meta type.
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void ctor(Op op) const ENTT_NOEXCEPT {
        internal::iterate([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node->ctor);
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
     * @brief Returns the meta destructor associated with a given type.
     * @return The meta destructor associated with the given type, if any.
     */
    inline meta_dtor * dtor() const ENTT_NOEXCEPT {
        return node->dtor ? node->dtor->meta() : nullptr;
    }

    /**
     * @brief Iterates all the meta data of a meta type.
     *
     * Iteratively returns **all** the meta data of the given type. This means
     * that the meta data of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void data(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::data>([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node);
    }

    /**
     * @brief Returns the meta data associated with a given name.
     *
     * Searches recursively among **all** the meta data of the given type. This
     * means that the meta data of the base classes will also be inspected, if
     * any.
     *
     * @param str The name to use to search for a meta data.
     * @return The meta data associated with the given name, if any.
     */
    inline meta_data * data(const char *str) const ENTT_NOEXCEPT {
        return internal::find_if<&internal::meta_type_node::data>([name = hashed_string{str}](auto *node) {
            return node->name == name;
        }, node);
    }

    /**
     * @brief Iterates all the meta functions of a meta type.
     *
     * Iteratively returns **all** the meta functions of the given type. This
     * means that the meta functions of the base classes will also be returned,
     * if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline void func(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::func>([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node);
    }

    /**
     * @brief Returns the meta function associated with a given name.
     *
     * Searches recursively among **all** the meta functions of the given type.
     * This means that the meta functions of the base classes will also be
     * inspected, if any.
     *
     * @param str The name to use to search for a meta function.
     * @return The meta function associated with the given name, if any.
     */
    inline meta_func * func(const char *str) const ENTT_NOEXCEPT {
        return internal::find_if<&internal::meta_type_node::func>([name = hashed_string{str}](auto *node) {
            return node->name == name;
        }, node);
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
        auto *ctor = internal::find_if<&internal::meta_type_node::ctor>([](auto *node) {
            return node->meta()->template accept<Args...>();
        }, node);

        return ctor ? ctor->invoke(std::forward<Args>(args)...) : meta_any{};
    }

    /**
     * @brief Destroys an instance of the underlying type.
     *
     * It must be possible to cast the instance to the underlying type.
     * Otherwise, invoking the meta destructor results in an undefined behavior.
     *
     * @param handle An opaque pointer to an instance of the underlying type.
     */
    inline void destroy(meta_handle handle) const {
        return node->dtor ? node->dtor->invoke(handle) : node->destroy(handle);
    }

    /**
     * @brief Iterates all the properties assigned to a meta type.
     *
     * Iteratively returns **all** the properties of the given type. This means
     * that the properties of the base classes will also be returned, if any.
     *
     * @tparam Op Type of the function object to invoke.
     * @param op A valid function object.
     */
    template<typename Op>
    inline std::enable_if_t<std::is_invocable_v<Op, meta_prop *>, void>
    prop(Op op) const ENTT_NOEXCEPT {
        internal::iterate<&internal::meta_type_node::prop>([op = std::move(op)](auto *node) {
            op(node->meta());
        }, node);
    }

    /**
     * @brief Returns the property associated with a given key.
     *
     * Searches recursively among **all** the properties of the given type. This
     * means that the properties of the base classes will also be inspected, if
     * any.
     *
     * @tparam Key Type of key to use to search for a property.
     * @param key The key to use to search for a property.
     * @return The property associated with the given key, if any.
     */
    template<typename Key>
    inline std::enable_if_t<!std::is_invocable_v<Key, meta_prop *>, meta_prop *>
    prop(Key &&key) const ENTT_NOEXCEPT {
        return internal::find_if<&internal::meta_type_node::prop>([key = std::forward<Key>(key)](auto *node) {
            const auto &id = node->meta()->key();
            return id.template can_cast<Key>() && id == key;
        }, node);
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

    inline static auto arg(typename meta_func_node::size_type index) {
        return std::array<meta_type_node *, sizeof...(Args)>{{meta_info<Args>::resolve()...}}[index];
    }

    static bool accept(const meta_type_node ** const types) {
        return accept(types, std::make_index_sequence<sizeof...(Args)>{});
    }

private:
    inline static bool can_cast_or_convert(const meta_type_node *from, const meta_type_node *to) ENTT_NOEXCEPT {
        return (from == to)
                || find_if<&meta_type_node::base>([to](auto *node) { return node->type() == to; }, from)
                || find_if<&meta_type_node::conv>([to](auto *node) { return node->type() == to; }, from);
    }

    template<std::size_t... Indexes>
    inline static auto accept(const meta_type_node ** const types, std::index_sequence<Indexes...>) {
        return (can_cast_or_convert(*(types+Indexes), meta_info<Args>::resolve()) && ...);
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
inline meta_handle handle([[maybe_unused]] void *inst) {
    if constexpr(std::is_void_v<Type>) {
        return meta_handle{};
    } else {
        return meta_handle{*static_cast<Type *>(inst)};
    }
}


template<typename Type>
inline void destroy([[maybe_unused]] meta_handle handle) {
    if constexpr(std::is_void_v<Type>) {
        assert(false);
    } else {
        assert(handle.type() == internal::meta_info<Type>::resolve()->meta());
        static_cast<Type *>(handle.data())->~Type();
    }
}


template<typename Type, typename... Args, std::size_t... Indexes>
inline meta_any construct(const meta_any * const any, std::index_sequence<Indexes...>) {
    // TODO cast or convert
    return meta_any{Type{(any+Indexes)->cast<std::decay_t<Args>>()...}};
}


template<bool Const, typename Type, auto Data>
inline void setter([[maybe_unused]] meta_handle handle, [[maybe_unused]] const meta_any &any) {
    // TODO cast or convert
    if constexpr(Const) {
        assert(false);
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        auto *instance = handle.try_cast<Type>();
        assert(instance);
        instance->*Data = any.cast<std::decay_t<decltype(std::declval<Type>().*Data)>>();
    } else {
        *Data = any.cast<std::decay_t<decltype(*Data)>>();
    }
}


template<typename Type, auto Data>
inline meta_any getter([[maybe_unused]] meta_handle handle) {
    // TODO cast or convert
    if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        auto *instance = handle.try_cast<Type>();
        assert(instance);
        return meta_any{handle.try_cast<Type>()->*Data};
    } else {
        return meta_any{*Data};
    }
}


template<auto Func, std::size_t... Indexes>
std::enable_if_t<std::is_function_v<std::remove_pointer_t<decltype(Func)>>, meta_any>
invoke(const meta_handle &, const meta_any *any, std::index_sequence<Indexes...>) {
    // TODO cast or convert
    using helper_type = meta_function_helper<std::integral_constant<decltype(Func), Func>>;

    if constexpr(std::is_void_v<typename helper_type::return_type>) {
        (*Func)((any+Indexes)->cast<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...);
        return meta_any{};
    } else {
        return meta_any{(*Func)((any+Indexes)->cast<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...)};
    }
}


template<auto Member, std::size_t... Indexes>
std::enable_if_t<std::is_member_function_pointer_v<decltype(Member)>, meta_any>
invoke(meta_handle &handle, const meta_any *any, std::index_sequence<Indexes...>) {
    // TODO cast or convert
    using helper_type = meta_function_helper<std::integral_constant<decltype(Member), Member>>;
    auto *clazz = handle.try_cast<typename helper_type::class_type>();
    assert(clazz);

    if constexpr(std::is_void_v<typename helper_type::return_type>) {
        (clazz->*Member)((any+Indexes)->cast<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...);
        return meta_any{};
    } else {
        return meta_any{(clazz->*Member)((any+Indexes)->cast<std::decay_t<std::tuple_element_t<Indexes, typename helper_type::args_type>>>()...)};
    }
}


template<typename Type>
meta_type_node * meta_node<Type>::resolve() ENTT_NOEXCEPT {
    if(!type) {
        static meta_type_node node{
            {},
            nullptr,
            nullptr,
            &handle<Type>,
            &destroy<Type>,
            []() {
                static meta_type meta{&node};
                return &meta;
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
