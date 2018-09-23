#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

enum class properties {
    prop_int,
    prop_bool
};

struct empty_type {};

struct fat_type {
    int *foo{nullptr};
    int *bar{nullptr};

    bool operator==(const fat_type &other) const {
        return foo == other.foo && bar == other.bar;
    }
};

bool operator!=(const fat_type &lhs, const fat_type &rhs) {
    return !(lhs == rhs);
}

struct base_type {};

struct derived_type: base_type {
    derived_type() = default;
    derived_type(int i, char c): i{i}, c{c} {}

    const int i{};
    const char c{};
};

derived_type derived_factory() {
    return {42, 'c'};
}

struct destroyable_type {
    ~destroyable_type() { ++counter; }
    inline static int counter = 0;
};

struct cleanup_type: destroyable_type {
    static void destroy(cleanup_type &instance) {
        instance.~cleanup_type();
        ++counter;
    }
};

struct not_comparable_type {
    bool operator==(const not_comparable_type &) const = delete;
};

bool operator!=(const not_comparable_type &, const not_comparable_type &) = delete;

struct Meta: public ::testing::Test {
    static void SetUpTestCase() {
        entt::reflect<char>("char", std::make_pair(properties::prop_int, 42));

        entt::reflect<base_type>("base");

        entt::reflect<derived_type>("derived")
                .base<base_type>()
                .ctor<int, char>(std::make_pair(properties::prop_bool, false))
                .ctor<&derived_factory>(std::make_pair(properties::prop_int, 42));

        entt::reflect<destroyable_type>("destroyable");

        entt::reflect<cleanup_type>("cleanup")
                .dtor<&cleanup_type::destroy>(std::make_pair(properties::prop_int, 42));
    }

    void SetUp() override {
        destroyable_type::counter = 0;
    }
};

TEST_F(Meta, Resolve) {
    ASSERT_EQ(entt::resolve<derived_type>(), entt::resolve("derived"));

    bool found = false;

    entt::resolve([&found](auto *type) {
        found = found || type == entt::resolve<derived_type>();
    });

    ASSERT_TRUE(found);
}

TEST_F(Meta, MetaHandle) {
    empty_type empty{};
    entt::meta_handle handle{empty};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<empty_type>());
    ASSERT_FALSE(handle.convertible<void>());
    ASSERT_TRUE(handle.convertible<empty_type>());
    ASSERT_EQ(handle.to<empty_type>(), &empty);
    ASSERT_EQ(std::as_const(handle).to<empty_type>(), &empty);
    ASSERT_EQ(handle.data(), &empty);
    ASSERT_EQ(std::as_const(handle).data(), &empty);
}

TEST_F(Meta, MetaHandleEmpty) {
    entt::meta_handle handle{};

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.type(), nullptr);
    ASSERT_FALSE(handle.convertible<void>());
    ASSERT_FALSE(handle.convertible<empty_type>());
    ASSERT_EQ(handle.data(), nullptr);
    ASSERT_EQ(std::as_const(handle).data(), nullptr);
}

TEST_F(Meta, MetaHandleConvertible) {
    derived_type derived{};
    base_type *base = &derived;
    entt::meta_handle handle{derived};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<derived_type>());
    ASSERT_FALSE(handle.convertible<void>());
    ASSERT_TRUE(handle.convertible<base_type>());
    ASSERT_TRUE(handle.convertible<derived_type>());
    ASSERT_EQ(handle.to<base_type>(), base);
    ASSERT_EQ(handle.to<derived_type>(), &derived);
    ASSERT_EQ(std::as_const(handle).to<base_type>(), base);
    ASSERT_EQ(std::as_const(handle).to<derived_type>(), &derived);
    ASSERT_EQ(handle.data(), &derived);
    ASSERT_EQ(std::as_const(handle).data(), &derived);
}

TEST_F(Meta, MetaAnySBO) {
    entt::meta_any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.handle());
    ASSERT_FALSE(any.convertible<void>());
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_EQ(any.to<char>(), 'c');
    ASSERT_EQ(std::as_const(any).to<char>(), 'c');
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(any, entt::meta_any{'h'});
}

TEST_F(Meta, MetaAnyNoSBO) {
    int value = 42;
    fat_type instance{&value, &value};
    entt::meta_any any{instance};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.handle());
    ASSERT_FALSE(any.convertible<void>());
    ASSERT_TRUE(any.convertible<fat_type>());
    ASSERT_EQ(any.to<fat_type>(), instance);
    ASSERT_EQ(std::as_const(any).to<fat_type>(), instance);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat_type{});
}

TEST_F(Meta, MetaAnyEmpty) {
    entt::meta_any any{};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.handle());
    ASSERT_EQ(any.type(), nullptr);
    ASSERT_FALSE(any.convertible<void>());
    ASSERT_FALSE(any.convertible<empty_type>());
    ASSERT_EQ(any, entt::meta_any{});
    ASSERT_NE(any, entt::meta_any{'c'});
}

TEST_F(Meta, MetaAnySBOMoveConstruction) {
    entt::meta_any any{42};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(other.handle());
    ASSERT_FALSE(other.convertible<void>());
    ASSERT_TRUE(other.convertible<int>());
    ASSERT_EQ(other.to<int>(), 42);
    ASSERT_EQ(std::as_const(other).to<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnyNoSBOMoveConstruction) {
    int value = 42;
    fat_type instance{&value, &value};
    entt::meta_any any{instance};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(other.handle());
    ASSERT_FALSE(other.convertible<void>());
    ASSERT_TRUE(other.convertible<fat_type>());
    ASSERT_EQ(other.to<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).to<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnySBOMoveAssignment) {
    entt::meta_any any{42};
    entt::meta_any other{};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(other.handle());
    ASSERT_FALSE(other.convertible<void>());
    ASSERT_TRUE(other.convertible<int>());
    ASSERT_EQ(other.to<int>(), 42);
    ASSERT_EQ(std::as_const(other).to<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnyNoSBOMoveAssignment) {
    int value = 42;
    fat_type instance{&value, &value};
    entt::meta_any any{instance};
    entt::meta_any other{};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(other.handle());
    ASSERT_FALSE(other.convertible<void>());
    ASSERT_TRUE(other.convertible<fat_type>());
    ASSERT_EQ(other.to<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).to<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyComparable) {
    entt::meta_any any{'c'};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(any, entt::meta_any{'a'});
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::meta_any{'c'});
    ASSERT_FALSE(any == entt::meta_any{'a'});
    ASSERT_TRUE(any != entt::meta_any{'a'});
    ASSERT_TRUE(any != entt::meta_any{});
}

TEST_F(Meta, MetaAnyNotComparable) {
    entt::meta_any any{not_comparable_type{}};

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::meta_any{not_comparable_type{}});
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::meta_any{not_comparable_type{}});
    ASSERT_TRUE(any != entt::meta_any{});
}

TEST_F(Meta, MetaAnyConvertible) {
    entt::meta_any any{derived_type{}};
    auto handle = any.handle();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<derived_type>());
    ASSERT_FALSE(any.convertible<void>());
    ASSERT_TRUE(any.convertible<base_type>());
    ASSERT_TRUE(any.convertible<derived_type>());
    ASSERT_EQ(&any.to<base_type>(), handle.to<base_type>());
    ASSERT_EQ(&any.to<derived_type>(), handle.to<derived_type>());
    ASSERT_EQ(&std::as_const(any).to<base_type>(), handle.to<base_type>());
    ASSERT_EQ(&std::as_const(any).to<derived_type>(), handle.to<derived_type>());
}

TEST_F(Meta, MetaProp) {
    auto *prop = entt::resolve<char>()->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 42);
}

TEST_F(Meta, MetaBase) {
    auto *base = entt::resolve<derived_type>()->base("base");
    derived_type derived{};

    ASSERT_NE(base, nullptr);
    ASSERT_EQ(base->parent(), entt::resolve("derived"));
    ASSERT_EQ(base->type(), entt::resolve<base_type>());
    ASSERT_EQ(base->convert(&derived), static_cast<base_type *>(&derived));
}

TEST_F(Meta, MetaCtor) {
    auto *ctor = entt::resolve<derived_type>()->ctor<int, char>();

    ASSERT_NE(ctor, nullptr);
    ASSERT_EQ(ctor->parent(), entt::resolve("derived"));
    ASSERT_EQ(ctor->size(), entt::meta_ctor::size_type{2});
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{1}), entt::resolve<char>());
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{2}), nullptr);
    ASSERT_TRUE((ctor->accept<int, char>()));
    ASSERT_FALSE((ctor->accept<>()));

    auto any = ctor->invoke(42, 'c');
    auto empty = ctor->invoke();

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<derived_type>());
    ASSERT_EQ(any.to<derived_type>().i, 42);
    ASSERT_EQ(any.to<derived_type>().c, 'c');

    ctor->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_EQ(prop->value(), false);
    });

    ASSERT_EQ(ctor->prop(properties::prop_int), nullptr);

    auto *prop = ctor->prop(properties::prop_bool);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_bool);
    ASSERT_EQ(prop->value(), false);
}

TEST_F(Meta, MetaCtorFunc) {
    auto *ctor = entt::resolve<derived_type>()->ctor<>();

    ASSERT_NE(ctor, nullptr);
    ASSERT_EQ(ctor->parent(), entt::resolve("derived"));
    ASSERT_EQ(ctor->size(), entt::meta_ctor::size_type{});
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{0}), nullptr);
    ASSERT_FALSE((ctor->accept<int, char>()));
    ASSERT_TRUE((ctor->accept<>()));

    auto any = ctor->invoke();
    auto empty = ctor->invoke(42, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<derived_type>());
    ASSERT_EQ(any.to<derived_type>().i, 42);
    ASSERT_EQ(any.to<derived_type>().c, 'c');

    ctor->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_int);
        ASSERT_EQ(prop->value(), 42);
    });

    ASSERT_EQ(ctor->prop(properties::prop_bool), nullptr);

    auto *prop = ctor->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 42);
}

TEST_F(Meta, MetaDtor) {
    auto *dtor = entt::resolve<cleanup_type>()->dtor();
    cleanup_type cleanup{};

    ASSERT_NE(dtor, nullptr);
    ASSERT_EQ(dtor->parent(), entt::resolve("cleanup"));
    ASSERT_EQ(destroyable_type::counter, 0);

    dtor->invoke(entt::meta_handle{cleanup});

    ASSERT_EQ(destroyable_type::counter, 2);

    dtor->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_int);
        ASSERT_EQ(prop->value(), 42);
    });

    ASSERT_EQ(dtor->prop(properties::prop_bool), nullptr);

    auto *prop = dtor->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 42);
}

TEST_F(Meta, MetaData) {
    // TODO
}

TEST_F(Meta, MetaDataConst) {
    // TODO
}

TEST_F(Meta, MetaDataStatic) {
    // TODO
}

TEST_F(Meta, MetaFunc) {
    // TODO
}

TEST_F(Meta, MetaFuncRetVoid) {
    // TODO
}

TEST_F(Meta, MetaFuncStatic) {
    // TODO
}

TEST_F(Meta, MetaFuncStaticRetVoid) {
    // TODO
}

TEST_F(Meta, MetaFuncWithDerivedTypes) {
    // TODO
}

TEST_F(Meta, MetaType) {
    // TODO
}

TEST_F(Meta, MetaTypeConstruct) {
    // TODO
}

TEST_F(Meta, MetaTypeDestroyNoDtor) {
    // TODO
}

TEST_F(Meta, MetaTypeDestroyWithDtor) {
    // TODO
}

TEST_F(Meta, AbstractClass) {
    //TODO
}

TEST_F(Meta, Convertibles) {
    // TODO
}

/*
template<typename Type>
struct helper_type {
    template<typename... Args>
    static Type ctor(Args... args) { return Type{args...}; }

    static void dtor(Type &v) {
        value = v;
    }

    static Type identity(Type v) {
        return v;
    }

    static void reset() {
        value = {};
    }

    static Type value;
};

template<typename Type>
Type helper_type<Type>::value{};

struct counter {
    counter() { ++value; }
    counter(const counter &) { ++value; }
    counter(counter &&) { ++value; }
    counter & operator=(const counter &) { ++value; return *this; }
    counter & operator=(counter &&) { ++value; return *this; }
    ~counter() { --value; }
    static int value;
};

int counter::value{};

struct abstract_class {
    virtual ~abstract_class() = default;
    virtual void do_nothing(const int &, const abstract_class &) const noexcept = 0;
};

struct base_class {
    virtual ~base_class() = default;
    virtual int square(int) const noexcept = 0;
    int div(int v) { return v/v; }
    void reset() { value = {}; }
    double value{0.};
};

struct derived_class: abstract_class, base_class {
    int square(int v) const noexcept override { return v*v; }
    void do_nothing(const int &, const abstract_class &) const noexcept override {}
    const double cvalue{0.};
};

enum class properties {
    bool_property,
    int_property
};

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::reflect<int>("int")
                .data<&helper_type<int>::value>("value")
                .func<&helper_type<int>::reset>("reset");

        entt::reflect<char>("char", std::make_pair(properties::bool_property, false), std::make_pair(properties::int_property, 3))
                .ctor<>(std::make_pair(properties::bool_property, true))
                .ctor<&helper_type<char>::ctor<char>>()
                .dtor<&helper_type<char>::dtor>(std::make_pair(properties::bool_property, false))
                .data<&helper_type<char>::value>("value", std::make_pair(properties::int_property, 1))
                .func<&helper_type<char>::identity>("identity", std::make_pair(properties::int_property, 99));

        entt::reflect<counter>("counter").ctor<>();

        entt::reflect<base_class>("base", std::make_pair(properties::int_property, 42))
                .func<&base_class::square>("square")
                .func<&base_class::div>("div")
                .func<&base_class::reset>("reset")
                .data<&base_class::value>("value");

        entt::reflect<derived_class>("derived")
                .ctor<>()
                .base<base_class>()
                .base<abstract_class>()
                .func<&derived_class::do_nothing>("noop")
                .data<&derived_class::cvalue>("cvalue");
    }
};

TEST_F(Meta, ResolveAll) {
    entt::resolve([](auto *type) {
        ASSERT_TRUE((type == entt::resolve("int"))
                    || (type == entt::resolve("char"))
                    || (type == entt::resolve("counter"))
                    || (type == entt::resolve("base"))
                    || (type == entt::resolve("derived")));
    });
}

TEST_F(Meta, Fundamental) {
    ASSERT_EQ(entt::resolve<char>(), entt::resolve("char"));
    ASSERT_NE(entt::resolve<char>(), nullptr);
    ASSERT_NE(entt::resolve("char"), nullptr);

    auto *type = entt::resolve<char>();

    ASSERT_STREQ(type->name(), "char");

    ASSERT_NE(type->ctor<>(), nullptr);
    ASSERT_NE(type->ctor<char>(), nullptr);
    ASSERT_EQ(type->ctor<int>(), nullptr);

    type->ctor([](auto *meta) {
        ASSERT_TRUE(meta->template accept<>() || meta->template accept<char>());
    });

    ASSERT_NE(type->dtor(), nullptr);
    ASSERT_NE(type->data("value"), nullptr);
    ASSERT_EQ(type->data("eulav"), nullptr);

    type->data([](auto *meta) {
        ASSERT_STREQ(meta->name(), "value");
    });

    ASSERT_NE(type->func("identity"), nullptr);
    ASSERT_EQ(type->func("ytitnedi"), nullptr);

    type->func([](auto *meta) {
        ASSERT_STREQ(meta->name(), "identity");
    });

    auto any = type->construct();

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_TRUE(any.convertible<const char>());
    ASSERT_TRUE(any.convertible<const char &>());
    ASSERT_EQ(any.type(), entt::resolve<char>());
    ASSERT_EQ(any.type(), entt::resolve<const char>());
    ASSERT_EQ(any.type(), entt::resolve<const char &>());
    ASSERT_NE(any.type(), nullptr);
    ASSERT_EQ(any.to<char>(), char{});

    any = type->construct('c');

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_TRUE(any.convertible<const char>());
    ASSERT_TRUE(any.convertible<const char &>());
    ASSERT_EQ(any.type(), entt::resolve<char>());
    ASSERT_EQ(any.type(), entt::resolve<const char>());
    ASSERT_EQ(any.type(), entt::resolve<const char &>());
    ASSERT_NE(any.type(), nullptr);
    ASSERT_EQ(any.to<char>(), 'c');

    ASSERT_EQ(helper_type<char>::value, char{});

    type->destroy(any.handle());

    ASSERT_EQ(helper_type<char>::value, 'c');
}

TEST_F(Meta, Struct) {
    ASSERT_EQ(entt::resolve<derived_class>(), entt::resolve("derived"));
    ASSERT_NE(entt::resolve<derived_class>(), nullptr);
    ASSERT_NE(entt::resolve("derived"), nullptr);

    auto *type = entt::resolve<derived_class>();
    derived_class derived;

    ASSERT_STREQ(type->name(), "derived");

    type->ctor([](auto *ctor) {
        ASSERT_EQ(ctor->size(), typename entt::meta_ctor::size_type{});
    });

    ASSERT_EQ(type->dtor(), nullptr);
    ASSERT_NE(type->base("base"), nullptr);
    ASSERT_EQ(type->base("esab"), nullptr);
    ASSERT_NE(type->data("value"), nullptr);
    ASSERT_EQ(type->data("eulav"), nullptr);
    ASSERT_NE(type->func("square"), nullptr);
    ASSERT_EQ(type->func("erauqs"), nullptr);
    ASSERT_NE(type->func("div"), nullptr);
    ASSERT_EQ(type->func("vid"), nullptr);
}

TEST_F(Meta, MetaHandle) {
    // TODO
}


// TODO create a test with convertibles


TEST_F(Meta, MetaAny) {
    entt::meta_any empty{};
    const entt::meta_any &cempty = empty;

    ASSERT_FALSE(empty);
    ASSERT_FALSE(cempty);
    ASSERT_EQ(empty.type(), nullptr);
    ASSERT_FALSE(empty.convertible<int>());
    ASSERT_FALSE(cempty.convertible<char>());
    ASSERT_EQ(cempty.handle().data(), nullptr);
    ASSERT_EQ(empty.handle().data(), nullptr);
    ASSERT_EQ(empty, cempty);

    entt::meta_any any{'c'};
    const entt::meta_any &cany = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(cany);
    ASSERT_NE(any.type(), nullptr);
    ASSERT_EQ(any.type(), cany.type());
    ASSERT_EQ(cany.type(), entt::resolve<char>());
    ASSERT_EQ(cany.type(), entt::resolve<char &>());
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_EQ(cany.to<char>(), any.to<char>());
    ASSERT_EQ(cany.to<char>(), 'c');
    ASSERT_NE(cany.handle().data(), nullptr);
    ASSERT_NE(any.handle().data(), nullptr);
    ASSERT_EQ(any, cany);
}

TEST_F(Meta, MetaAnyCompare) {
    entt::meta_any ch1{'c'};
    entt::meta_any ch2{'c'};
    entt::meta_any ch3{'#'};

    ASSERT_EQ(ch1, ch2);
    ASSERT_NE(ch1, ch3);

    entt::meta_any d1{derived_class{}};
    entt::meta_any d2{derived_class{}};

    ASSERT_NE(d1, d2);

    // damn gtest, it doesn't invoke operator== under the hood apparently
    ASSERT_TRUE(ch1 == ch2);
    ASSERT_FALSE(ch1 == ch3);
    ASSERT_FALSE(d1 == d2);
}

TEST_F(Meta, MetaAnyConvertible) {
    entt::meta_any any{derived_class{}};
    const entt::meta_any &cany = any;

    const void *derived = cany.handle().to<derived_class>();
    const void *abstract = cany.handle().to<abstract_class>();
    const void *base = cany.handle().to<base_class>();

    ASSERT_TRUE(cany);
    ASSERT_TRUE(any);

    ASSERT_TRUE(cany.convertible<base_class>());
    ASSERT_TRUE(any.convertible<abstract_class>());

    ASSERT_EQ(cany.handle().to<derived_class>(), derived);
    ASSERT_EQ(cany.handle().to<abstract_class>(), abstract);
    ASSERT_EQ(cany.handle().to<base_class>(), base);

    ASSERT_EQ(any.handle().to<derived_class>(), derived);
    ASSERT_EQ(any.handle().to<abstract_class>(), abstract);
    ASSERT_EQ(any.handle().to<base_class>(), base);
}

TEST_F(Meta, MetaProp) {
    auto *type = entt::resolve<char>();

    ASSERT_NE(type->prop(properties::bool_property), nullptr);
    ASSERT_NE(type->prop(properties::int_property), nullptr);
    ASSERT_TRUE(type->prop(properties::bool_property)->key().convertible<properties>());
    ASSERT_TRUE(type->prop(properties::int_property)->key().convertible<properties>());
    ASSERT_EQ(type->prop(properties::bool_property)->key().to<properties>(), properties::bool_property);
    ASSERT_EQ(type->prop(properties::int_property)->key().to<properties>(), properties::int_property);
    ASSERT_TRUE(type->prop(properties::bool_property)->value().convertible<bool>());
    ASSERT_TRUE(type->prop(properties::int_property)->value().convertible<int>());
    ASSERT_FALSE(type->prop(properties::bool_property)->value().to<bool>());
    ASSERT_EQ(type->prop(properties::int_property)->value().to<int>(), 3);

    type->prop([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<properties>());

        if(prop->value().template convertible<bool>()) {
            ASSERT_FALSE(prop->value().template to<bool>());
        } else if(prop->value().template convertible<int>()) {
            ASSERT_EQ(prop->value().template to<int>(), 3);
        } else {
            FAIL();
        }
    });
}

TEST_F(Meta, MetaCtor) {
    auto *ctor = entt::resolve<char>()->ctor<char>();

    ASSERT_NE(ctor, nullptr);
    ASSERT_EQ(ctor->parent(), entt::resolve<char>());

    ASSERT_EQ(ctor->size(), typename entt::meta_ctor::size_type{1});
    ASSERT_EQ(ctor->arg({}), entt::resolve<char>());
    ASSERT_NE(ctor->arg({}), entt::resolve<int>());
    ASSERT_EQ(ctor->arg(typename entt::meta_ctor::size_type{1}), nullptr);
    ASSERT_FALSE(ctor->accept<>());
    ASSERT_FALSE(ctor->accept<int>());
    ASSERT_TRUE(ctor->accept<char>());
    ASSERT_TRUE(ctor->accept<const char>());
    ASSERT_TRUE(ctor->accept<const char &>());

    auto ok = ctor->invoke('c');
    auto ko = ctor->invoke(42);

    ASSERT_FALSE(ko);
    ASSERT_TRUE(ok);
    ASSERT_NE(ok, ko);
    ASSERT_FALSE(ko.convertible<char>());
    ASSERT_FALSE(ko.convertible<int>());
    ASSERT_TRUE(ok.convertible<char>());
    ASSERT_FALSE(ok.convertible<int>());
    ASSERT_EQ(ok.to<char>(), 'c');

    ctor = entt::resolve<char>()->ctor<>();

    ASSERT_NE(ctor->prop(properties::bool_property), nullptr);
    ASSERT_TRUE(ctor->prop(properties::bool_property)->key().convertible<properties>());
    ASSERT_EQ(ctor->prop(properties::bool_property)->key().to<properties>(), properties::bool_property);
    ASSERT_TRUE(ctor->prop(properties::bool_property)->value().convertible<bool>());
    ASSERT_TRUE(ctor->prop(properties::bool_property)->value().to<bool>());
    ASSERT_EQ(ctor->prop(properties::int_property), nullptr);

    ctor->prop([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<properties>());
        ASSERT_EQ(prop->key().template to<properties>(), properties::bool_property);
        ASSERT_TRUE(prop->value().template convertible<bool>());
        ASSERT_TRUE(prop->value().template to<bool>());
    });
}

TEST_F(Meta, MetaDtor) {
    auto *dtor = entt::resolve<char>()->dtor();
    char ch = '*';

    ASSERT_NE(dtor, nullptr);
    ASSERT_EQ(dtor->parent(), entt::resolve<char>());

    ASSERT_NE(helper_type<char>::value, '*');

    dtor->invoke(ch);

    ASSERT_EQ(helper_type<char>::value, '*');

    ASSERT_NE(dtor->prop(properties::bool_property), nullptr);
    ASSERT_TRUE(dtor->prop(properties::bool_property)->key().convertible<properties>());
    ASSERT_EQ(dtor->prop(properties::bool_property)->key().to<properties>(), properties::bool_property);
    ASSERT_TRUE(dtor->prop(properties::bool_property)->value().convertible<bool>());
    ASSERT_FALSE(dtor->prop(properties::bool_property)->value().to<bool>());
    ASSERT_EQ(dtor->prop(properties::int_property), nullptr);

    dtor->prop([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<properties>());
        ASSERT_EQ(prop->key().template to<properties>(), properties::bool_property);
        ASSERT_TRUE(prop->value().template convertible<bool>());
        ASSERT_FALSE(prop->value().template to<bool>());
    });
}

TEST_F(Meta, MetaData) {
    auto *data = entt::resolve<derived_class>()->data("value");
    const auto *cdata = entt::resolve<derived_class>()->data("cvalue");
    derived_class derived;

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->parent(), entt::resolve<base_class>());

    ASSERT_NE(cdata, nullptr);
    ASSERT_EQ(cdata->parent(), entt::resolve<derived_class>());

    ASSERT_STREQ(data->name(), "value");
    ASSERT_FALSE(data->is_const());
    ASSERT_FALSE(data->is_static());
    ASSERT_EQ(data->type(), entt::resolve<double>());

    ASSERT_STREQ(cdata->name(), "cvalue");
    ASSERT_TRUE(cdata->is_const());
    ASSERT_FALSE(cdata->is_static());
    ASSERT_EQ(cdata->type(), entt::resolve<double>());

    ASSERT_TRUE(data->accept<double>());
    ASSERT_TRUE(data->accept<const double>());
    ASSERT_TRUE(data->accept<const double &>());
    ASSERT_FALSE(data->accept<double *>());
    ASSERT_FALSE(data->accept<int>());

    data->set(derived, 3.);
    const auto any = data->get(derived);

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<double>());
    ASSERT_EQ(any.to<double>(), 3.);

    ASSERT_EQ(data->prop(properties::int_property), nullptr);
    ASSERT_EQ(data->prop(properties::bool_property), nullptr);

    data->prop([](auto *) {
        FAIL();
    });
}

TEST_F(Meta, MetaDataStatic) {
    auto *data = entt::resolve<char>()->data("value");

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->parent(), entt::resolve<char>());

    ASSERT_STREQ(data->name(), "value");
    ASSERT_FALSE(data->is_const());
    ASSERT_TRUE(data->is_static());
    ASSERT_EQ(data->type(), entt::resolve<char>());

    ASSERT_TRUE(data->accept<char>());
    ASSERT_TRUE(data->accept<const char>());
    ASSERT_TRUE(data->accept<const char &>());
    ASSERT_FALSE(data->accept<char *>());
    ASSERT_FALSE(data->accept<int>());

    data->set(entt::meta_handle{}, '#');
    const auto any = data->get(entt::meta_handle{});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_EQ(any.to<char>(), '#');

    ASSERT_NE(data->prop(properties::int_property), nullptr);
    ASSERT_TRUE(data->prop(properties::int_property)->key().convertible<properties>());
    ASSERT_EQ(data->prop(properties::int_property)->key().to<properties>(), properties::int_property);
    ASSERT_TRUE(data->prop(properties::int_property)->value().convertible<int>());
    ASSERT_EQ(data->prop(properties::int_property)->value().to<int>(), 1);
    ASSERT_EQ(data->prop(properties::bool_property), nullptr);

    data->prop([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<properties>());
        ASSERT_EQ(prop->key().template to<properties>(), properties::int_property);
        ASSERT_TRUE(prop->value().template convertible<int>());
        ASSERT_EQ(prop->value().template to<int>(), 1);
    });
}

TEST_F(Meta, MetaFunc) {
    auto *func = entt::resolve<derived_class>()->func("square");
    const auto *cfunc = func;
    derived_class derived;

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve<base_class>());

    ASSERT_NE(cfunc, nullptr);
    ASSERT_EQ(cfunc->parent(), entt::resolve<base_class>());

    ASSERT_STREQ(func->name(), "square");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{1});
    ASSERT_TRUE(func->is_const());
    ASSERT_FALSE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<int>());
    ASSERT_EQ(func->arg({}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), nullptr);

    ASSERT_TRUE(func->accept<int>());
    ASSERT_TRUE(func->accept<const int>());
    ASSERT_TRUE(func->accept<const int &>());
    ASSERT_FALSE(func->accept<int *>());
    ASSERT_FALSE(func->accept<double>());

    ASSERT_FALSE(cfunc->invoke(derived, '.'));
    ASSERT_TRUE(cfunc->invoke(derived, 0));
    ASSERT_FALSE(func->invoke(derived, '.'));
    ASSERT_TRUE(func->invoke(derived, 0));

    const auto any = func->invoke(derived, 2);

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<int>());
    ASSERT_EQ(any.to<int>(), 4);

    ASSERT_EQ(func->prop(properties::int_property), nullptr);
    ASSERT_EQ(func->prop(properties::bool_property), nullptr);

    func->prop([](auto *) {
        FAIL();
    });
}

TEST_F(Meta, MetaFuncVoid) {
    auto *data = entt::resolve<base_class>()->data("value");
    auto *cfunc = entt::resolve("derived")->func("noop");
    auto *func = entt::resolve("base")->func("reset");
    derived_class derived;

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve<base_class>());

    ASSERT_NE(cfunc, nullptr);
    ASSERT_EQ(cfunc->parent(), entt::resolve("derived"));

    ASSERT_STREQ(func->name(), "reset");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{});
    ASSERT_FALSE(func->is_const());
    ASSERT_FALSE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<void>());
    ASSERT_EQ(func->arg({}), nullptr);

    ASSERT_STREQ(cfunc->name(), "noop");
    ASSERT_EQ(cfunc->size(), entt::meta_func::size_type{2});
    ASSERT_TRUE(cfunc->is_const());
    ASSERT_FALSE(cfunc->is_static());
    ASSERT_EQ(cfunc->ret(), entt::resolve<void>());
    ASSERT_EQ(cfunc->arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(cfunc->arg(entt::meta_func::size_type{1}), entt::resolve<abstract_class>());
    ASSERT_EQ(cfunc->arg(entt::meta_func::size_type{2}), nullptr);

    ASSERT_TRUE(func->accept<>());
    ASSERT_FALSE(func->accept<void>());
    ASSERT_FALSE(func->accept<int>());
    ASSERT_FALSE(func->accept<const int>());
    ASSERT_FALSE(func->accept<const int &>());
    ASSERT_FALSE(func->accept<int *>());
    ASSERT_FALSE(func->accept<double>());

    ASSERT_FALSE(cfunc->accept<>());
    ASSERT_FALSE(cfunc->accept<void>());
    ASSERT_FALSE(cfunc->accept<int>());
    ASSERT_FALSE(cfunc->accept<const int>());
    ASSERT_FALSE(cfunc->accept<const int &>());
    ASSERT_FALSE(cfunc->accept<int *>());
    ASSERT_FALSE(cfunc->accept<double>());
    ASSERT_TRUE((cfunc->accept<int, abstract_class>()));
    ASSERT_TRUE((cfunc->accept<int, derived_class>()));

    data->set(derived, 1.2);

    ASSERT_EQ(data->get(derived).to<double>(), 1.2);

    auto any = func->invoke(derived);
    cfunc->invoke(derived, 0, derived);

    ASSERT_EQ(data->get(derived).to<double>(), 0.);
    ASSERT_FALSE(any);
}

TEST_F(Meta, MetaFuncStatic) {
    auto *func = entt::resolve<char>()->func("identity");
    const auto *cfunc = func;

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve<char>());

    ASSERT_NE(cfunc, nullptr);
    ASSERT_EQ(cfunc->parent(), entt::resolve<char>());

    ASSERT_STREQ(func->name(), "identity");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func->is_const());
    ASSERT_TRUE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<char>());
    ASSERT_EQ(func->arg({}), entt::resolve<char>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), nullptr);

    ASSERT_TRUE(func->accept<char>());
    ASSERT_TRUE(func->accept<const char>());
    ASSERT_TRUE(func->accept<const char &>());
    ASSERT_FALSE(func->accept<char *>());
    ASSERT_FALSE(func->accept<int>());

    ASSERT_FALSE(cfunc->invoke(entt::meta_handle{}, 0));
    ASSERT_TRUE(cfunc->invoke(entt::meta_handle{}, '.'));
    ASSERT_FALSE(func->invoke(entt::meta_handle{}, 0));
    ASSERT_TRUE(func->invoke(entt::meta_handle{}, '.'));

    const auto any = func->invoke(entt::meta_handle{}, '.');

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_EQ(any.to<char>(), '.');

    ASSERT_NE(func->prop(properties::int_property), nullptr);
    ASSERT_TRUE(func->prop(properties::int_property)->key().convertible<properties>());
    ASSERT_EQ(func->prop(properties::int_property)->key().to<properties>(), properties::int_property);
    ASSERT_TRUE(func->prop(properties::int_property)->value().convertible<int>());
    ASSERT_EQ(func->prop(properties::int_property)->value().to<int>(), 99);
    ASSERT_EQ(func->prop(properties::bool_property), nullptr);

    func->prop([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<properties>());
        ASSERT_EQ(prop->key().template to<properties>(), properties::int_property);
        ASSERT_TRUE(prop->value().template convertible<int>());
        ASSERT_EQ(prop->value().template to<int>(), 99);
    });
}

TEST_F(Meta, MetaFuncStaticVoid) {
    auto *data = entt::resolve<int>()->data("value");
    auto *func = entt::resolve("int")->func("reset");

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve<int>());

    ASSERT_STREQ(func->name(), "reset");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{});
    ASSERT_FALSE(func->is_const());
    ASSERT_TRUE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<void>());
    ASSERT_EQ(func->arg({}), nullptr);

    ASSERT_TRUE(func->accept<>());
    ASSERT_FALSE(func->accept<void>());
    ASSERT_FALSE(func->accept<int>());
    ASSERT_FALSE(func->accept<const int>());
    ASSERT_FALSE(func->accept<const int &>());
    ASSERT_FALSE(func->accept<int *>());
    ASSERT_FALSE(func->accept<double>());

    data->set(entt::meta_handle{}, 128);

    ASSERT_EQ(data->get(entt::meta_handle{}).to<int>(), 128);

    auto any = func->invoke(entt::meta_handle{});

    ASSERT_EQ(data->get(entt::meta_handle{}).to<int>(), 0);
    ASSERT_FALSE(any);
}

TEST_F(Meta, MetaType) {
    auto *type = entt::resolve("char");

    ASSERT_STREQ(type->name(), "char");

    ASSERT_NE(type->ctor<>(), nullptr);
    ASSERT_NE(type->ctor<char>(), nullptr);
    ASSERT_EQ(type->ctor<int>(), nullptr);

    type->ctor([](auto *ctor) {
        ASSERT_TRUE(ctor->template accept<>() || ctor->template accept<char>());
    });

    ASSERT_NE(type->dtor(), nullptr);
    ASSERT_NE(type->data("value"), nullptr);
    ASSERT_EQ(type->data("identity"), nullptr);

    type->data([](auto *data) {
        ASSERT_STREQ(data->name(), "value");
    });

    ASSERT_EQ(type->func("value"), nullptr);
    ASSERT_NE(type->func("identity"), nullptr);

    type->func([](auto *func) {
        ASSERT_STREQ(func->name(), "identity");
    });

    auto any = type->construct('c');

    ASSERT_EQ(any.to<char>(), 'c');

    helper_type<char>::value = '*';
    type->destroy(any.to<char>());

    ASSERT_EQ(helper_type<char>::value, 'c');

    ASSERT_NE(type->prop(properties::bool_property), nullptr);
    ASSERT_NE(type->prop(properties::int_property), nullptr);
    ASSERT_TRUE(type->prop(properties::bool_property)->key().convertible<properties>());
    ASSERT_TRUE(type->prop(properties::int_property)->key().convertible<properties>());
    ASSERT_EQ(type->prop(properties::bool_property)->key().to<properties>(), properties::bool_property);
    ASSERT_EQ(type->prop(properties::int_property)->key().to<properties>(), properties::int_property);
    ASSERT_TRUE(type->prop(properties::bool_property)->value().convertible<bool>());
    ASSERT_TRUE(type->prop(properties::int_property)->value().convertible<int>());
    ASSERT_FALSE(type->prop(properties::bool_property)->value().to<bool>());
    ASSERT_EQ(type->prop(properties::int_property)->value().to<int>(), 3);

    type->prop([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<properties>());

        if(prop->value().template convertible<bool>()) {
            ASSERT_FALSE(prop->value().template to<bool>());
        } else if(prop->value().template convertible<int>()) {
            ASSERT_EQ(prop->value().template to<int>(), 3);
        } else {
            FAIL();
        }
    });
}

TEST_F(Meta, DefDestructor) {
    auto *type = entt::resolve("counter");

    ASSERT_EQ(counter::value, 0);

    auto any = type->construct();

    ASSERT_TRUE(any);
    ASSERT_EQ(counter::value, 1);

    type->destroy(any.to<counter>());

    ASSERT_EQ(counter::value, 0);
}

TEST_F(Meta, BaseDerivedClasses) {
    auto *abstract = entt::resolve<abstract_class>();
    auto *base = entt::resolve("base");
    auto *derived = entt::resolve("derived");
    derived_class instance;

    ASSERT_NE(base, nullptr);
    ASSERT_NE(derived, nullptr);

    ASSERT_EQ(base->func("square")->invoke(instance, 2).to<int>(), 4);
    ASSERT_EQ(derived->func("square")->invoke(instance, 3).to<int>(), 9);

    ASSERT_EQ(base->func("div")->invoke(instance, 10).to<int>(), 1);
    ASSERT_EQ(derived->func("div")->invoke(instance, 99).to<int>(), 1);

    ASSERT_NE(derived->base("base"), nullptr);

    derived->base([base, abstract, derived](auto *curr) {
        ASSERT_TRUE(curr->type() == base || curr->type() == abstract);
        ASSERT_EQ(curr->parent(), derived);
    });

    auto any = derived->construct();

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<derived_class>());
    ASSERT_TRUE(any.convertible<base_class>());

    derived->data("value")->set(any.handle(), base->data("value")->get(any.handle()).to<double>() + .1);

    ASSERT_EQ(any.to<base_class>().value, .1);
    ASSERT_EQ(any.to<derived_class>().value, .1);

    ASSERT_NE(base->prop(properties::int_property), nullptr);
    ASSERT_NE(derived->prop(properties::int_property), nullptr);
    ASSERT_EQ(base->prop(properties::int_property), derived->prop(properties::int_property));
    ASSERT_EQ(derived->prop(properties::int_property)->value().to<int>(), 42);
}
*/
