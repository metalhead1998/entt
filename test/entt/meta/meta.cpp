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

struct data_type {
    int i{0};
    const int j{1};
    inline static int h{2};
    inline static const int k{3};
};

struct func_type {
    int f(int a, int b) { value = a; return b*b; }
    int f(int v) const { return v*v; }
    void g(int v) { value = v*v; }

    static int h(int v) { return v; }
    static void k(int v) { value = v; }

    inline static int value = 0;
};

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

        entt::reflect<data_type>("data")
                .data<&data_type::i>("i", std::make_pair(properties::prop_int, 0))
                .data<&data_type::j>("j", std::make_pair(properties::prop_int, 1))
                .data<&data_type::h>("h", std::make_pair(properties::prop_int, 2))
                .data<&data_type::k>("k", std::make_pair(properties::prop_int, 3));

        entt::reflect<func_type>("func")
                .func<entt::overload<int(int, int)>(&func_type::f)>("f2", std::make_pair(properties::prop_bool, false))
                .func<entt::overload<int(int) const>(&func_type::f)>("f1", std::make_pair(properties::prop_bool, false))
                .func<&func_type::g>("g", std::make_pair(properties::prop_bool, false))
                .func<&func_type::h>("h", std::make_pair(properties::prop_bool, false))
                .func<&func_type::k>("k", std::make_pair(properties::prop_bool, false));
    }

    void SetUp() override {
        destroyable_type::counter = 0;
        func_type::value = 0;
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

TEST_F(Meta, MetaAnySBO) {
    entt::meta_any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_TRUE(any.can_cast<char>());
    ASSERT_EQ(any.cast<char>(), 'c');
    ASSERT_EQ(std::as_const(any).cast<char>(), 'c');
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(any, entt::meta_any{'h'});
}

TEST_F(Meta, MetaAnyNoSBO) {
    int value = 42;
    fat_type instance{&value, &value};
    entt::meta_any any{instance};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_TRUE(any.can_cast<fat_type>());
    ASSERT_EQ(any.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(any).cast<fat_type>(), instance);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat_type{});
}

TEST_F(Meta, MetaAnyEmpty) {
    entt::meta_any any{};

    ASSERT_FALSE(any);
    ASSERT_EQ(any.type(), nullptr);
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_FALSE(any.can_cast<empty_type>());
    ASSERT_EQ(any, entt::meta_any{});
    ASSERT_NE(any, entt::meta_any{'c'});
}

TEST_F(Meta, MetaAnySBOCopyConstruction) {
    // TODO
}

TEST_F(Meta, MetaAnySBOCopyAssignment) {
    // TODO
}

TEST_F(Meta, MetaAnySBOMoveConstruction) {
    entt::meta_any any{42};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<int>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(std::as_const(other).cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOMoveAssignment) {
    entt::meta_any any{42};
    entt::meta_any other{};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<int>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(std::as_const(other).cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBODestruction) {
    // TODO
}

TEST_F(Meta, MetaAnyNoSBOCopyConstruction) {
    // TODO
}

TEST_F(Meta, MetaAnyNoSBOCopyAssignment) {
    // TODO
}

TEST_F(Meta, MetaAnyNoSBOMoveConstruction) {
    int value = 42;
    fat_type instance{&value, &value};
    entt::meta_any any{instance};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<fat_type>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBOMoveAssignment) {
    int value = 42;
    fat_type instance{&value, &value};
    entt::meta_any any{instance};
    entt::meta_any other{};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<fat_type>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBODestruction) {
    // TODO
}

TEST_F(Meta, MetaAnySwap) {
    // TODO
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

TEST_F(Meta, MetaAnyCast) {
    entt::meta_any any{derived_type{}};
    entt::meta_handle handle{any};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<derived_type>());
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_TRUE(any.can_cast<base_type>());
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(&any.cast<base_type>(), handle.try_cast<base_type>());
    ASSERT_EQ(&any.cast<derived_type>(), handle.try_cast<derived_type>());
    ASSERT_EQ(&std::as_const(any).cast<base_type>(), handle.try_cast<base_type>());
    ASSERT_EQ(&std::as_const(any).cast<derived_type>(), handle.try_cast<derived_type>());
}

TEST_F(Meta, MetaAnyConvert) {
    // TODO
}

TEST_F(Meta, MetaAnyConstConvert) {
    // TODO
}

TEST_F(Meta, MetaHandleFromObject) {
    empty_type empty{};
    entt::meta_handle handle{empty};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<empty_type>());
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<empty_type>(), &empty);
    ASSERT_EQ(std::as_const(handle).try_cast<empty_type>(), &empty);
    ASSERT_EQ(handle.data(), &empty);
    ASSERT_EQ(std::as_const(handle).data(), &empty);
}

TEST_F(Meta, MetaHandleFromMetaAny) {
    // TODO
}

TEST_F(Meta, MetaHandleEmpty) {
    entt::meta_handle handle{};

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.type(), nullptr);
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<empty_type>(), nullptr);
    ASSERT_EQ(handle.data(), nullptr);
    ASSERT_EQ(std::as_const(handle).data(), nullptr);
}

TEST_F(Meta, MetaHandleTryCast) {
    derived_type derived{};
    base_type *base = &derived;
    entt::meta_handle handle{derived};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<derived_type>());
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<base_type>(), base);
    ASSERT_EQ(handle.try_cast<derived_type>(), &derived);
    ASSERT_EQ(std::as_const(handle).try_cast<base_type>(), base);
    ASSERT_EQ(std::as_const(handle).try_cast<derived_type>(), &derived);
    ASSERT_EQ(handle.data(), &derived);
    ASSERT_EQ(std::as_const(handle).data(), &derived);
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
    ASSERT_EQ(base->cast(&derived), static_cast<base_type *>(&derived));
}

TEST_F(Meta, MetaConv) {
    // TODO
}

TEST_F(Meta, MetaCtor) {
    auto *ctor = entt::resolve<derived_type>()->ctor<int, char>();

    ASSERT_NE(ctor, nullptr);
    ASSERT_EQ(ctor->parent(), entt::resolve("derived"));
    ASSERT_EQ(ctor->size(), entt::meta_ctor::size_type{2});
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{1}), entt::resolve<char>());
    ASSERT_EQ(ctor->arg(entt::meta_ctor::size_type{2}), nullptr);

    auto any = ctor->invoke(42, 'c');
    auto empty = ctor->invoke();

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');

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

    auto any = ctor->invoke();
    auto empty = ctor->invoke(42, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');

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

TEST_F(Meta, MetaCtorMetaAnyArgs) {
    // TODO
}

TEST_F(Meta, MetaCtorInvalidArgs) {
    // TODO
}

TEST_F(Meta, MetaCtorCastAndConvert) {
    // TODO
}

TEST_F(Meta, MetaDtor) {
    auto *dtor = entt::resolve<cleanup_type>()->dtor();
    cleanup_type cleanup{};

    ASSERT_NE(dtor, nullptr);
    ASSERT_EQ(dtor->parent(), entt::resolve("cleanup"));
    ASSERT_EQ(destroyable_type::counter, 0);

    dtor->invoke(cleanup);

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

TEST_F(Meta, MetaDtorMetaAnyArg) {
    // TODO
}

TEST_F(Meta, MetaData) {
    auto *data = entt::resolve<data_type>()->data("i");
    data_type instance{};

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->parent(), entt::resolve("data"));
    ASSERT_EQ(data->type(), entt::resolve<int>());
    ASSERT_STREQ(data->name(), "i");
    ASSERT_FALSE(data->is_const());
    ASSERT_FALSE(data->is_static());
    ASSERT_EQ(data->get(instance).cast<int>(), 0);

    data->set(instance, 42);

    ASSERT_EQ(data->get(instance).cast<int>(), 42);

    data->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_int);
        ASSERT_EQ(prop->value(), 0);
    });

    ASSERT_EQ(data->prop(properties::prop_bool), nullptr);

    auto *prop = data->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 0);
}

TEST_F(Meta, MetaDataConst) {
    auto *data = entt::resolve<data_type>()->data("j");
    data_type instance{};

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->parent(), entt::resolve("data"));
    ASSERT_EQ(data->type(), entt::resolve<int>());
    ASSERT_STREQ(data->name(), "j");
    ASSERT_TRUE(data->is_const());
    ASSERT_FALSE(data->is_static());
    ASSERT_EQ(data->get(instance).cast<int>(), 1);

    data->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_int);
        ASSERT_EQ(prop->value(), 1);
    });

    ASSERT_EQ(data->prop(properties::prop_bool), nullptr);

    auto *prop = data->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 1);
}

TEST_F(Meta, MetaDataStatic) {
    auto *data = entt::resolve<data_type>()->data("h");

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->parent(), entt::resolve("data"));
    ASSERT_EQ(data->type(), entt::resolve<int>());
    ASSERT_STREQ(data->name(), "h");
    ASSERT_FALSE(data->is_const());
    ASSERT_TRUE(data->is_static());
    ASSERT_EQ(data->get({}).cast<int>(), 2);

    data->set({}, 42);

    ASSERT_EQ(data->get({}).cast<int>(), 42);

    data->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_int);
        ASSERT_EQ(prop->value(), 2);
    });

    ASSERT_EQ(data->prop(properties::prop_bool), nullptr);

    auto *prop = data->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 2);
}

TEST_F(Meta, MetaDataConstStatic) {
    auto *data = entt::resolve<data_type>()->data("k");

    ASSERT_NE(data, nullptr);
    ASSERT_EQ(data->parent(), entt::resolve("data"));
    ASSERT_EQ(data->type(), entt::resolve<int>());
    ASSERT_STREQ(data->name(), "k");
    ASSERT_TRUE(data->is_const());
    ASSERT_TRUE(data->is_static());
    ASSERT_EQ(data->get({}).cast<int>(), 3);

    data->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_int);
        ASSERT_EQ(prop->value(), 3);
    });

    ASSERT_EQ(data->prop(properties::prop_bool), nullptr);

    auto *prop = data->prop(properties::prop_int);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_int);
    ASSERT_EQ(prop->value(), 3);
}

TEST_F(Meta, MetaDataMetaAnyArg) {
    // TODO
}

TEST_F(Meta, MetaDataInvalidArg) {
    // TODO
}

TEST_F(Meta, MetaDataCastAndConvert) {
    // TODO
}

TEST_F(Meta, MetaFunc) {
    auto *func = entt::resolve<func_type>()->func("f2");
    func_type instance{};

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve("func"));
    ASSERT_STREQ(func->name(), "f2");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{2});
    ASSERT_FALSE(func->is_const());
    ASSERT_FALSE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{2}), nullptr);

    auto any = func->invoke(instance, 3, 2);
    auto empty = func->invoke(instance);

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 4);
    ASSERT_EQ(func_type::value, 3);

    func->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_EQ(func->prop(properties::prop_int), nullptr);

    auto *prop = func->prop(properties::prop_bool);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_bool);
    ASSERT_FALSE(prop->value().cast<bool>());
}

TEST_F(Meta, MetaFuncConst) {
    auto *func = entt::resolve<func_type>()->func("f1");
    func_type instance{};

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve("func"));
    ASSERT_STREQ(func->name(), "f1");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{1});
    ASSERT_TRUE(func->is_const());
    ASSERT_FALSE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), nullptr);

    auto any = func->invoke(instance, 4);
    auto empty = func->invoke(instance, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 16);

    func->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_EQ(func->prop(properties::prop_int), nullptr);

    auto *prop = func->prop(properties::prop_bool);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_bool);
    ASSERT_FALSE(prop->value().cast<bool>());
}

TEST_F(Meta, MetaFuncRetVoid) {
    auto *func = entt::resolve<func_type>()->func("g");
    func_type instance{};

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve("func"));
    ASSERT_STREQ(func->name(), "g");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func->is_const());
    ASSERT_FALSE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<void>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), nullptr);

    auto any = func->invoke(instance, 5);

    ASSERT_FALSE(any);
    ASSERT_EQ(func_type::value, 25);

    func->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_EQ(func->prop(properties::prop_int), nullptr);

    auto *prop = func->prop(properties::prop_bool);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_bool);
    ASSERT_FALSE(prop->value().cast<bool>());
}

TEST_F(Meta, MetaFuncStatic) {
    auto *func = entt::resolve<func_type>()->func("h");

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve("func"));
    ASSERT_STREQ(func->name(), "h");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func->is_const());
    ASSERT_TRUE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), nullptr);

    auto any = func->invoke({}, 42);
    auto empty = func->invoke({}, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 42);

    func->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_EQ(func->prop(properties::prop_int), nullptr);

    auto *prop = func->prop(properties::prop_bool);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_bool);
    ASSERT_FALSE(prop->value().cast<bool>());
}

TEST_F(Meta, MetaFuncStaticRetVoid) {
    auto *func = entt::resolve<func_type>()->func("k");

    ASSERT_NE(func, nullptr);
    ASSERT_EQ(func->parent(), entt::resolve("func"));
    ASSERT_STREQ(func->name(), "k");
    ASSERT_EQ(func->size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func->is_const());
    ASSERT_TRUE(func->is_static());
    ASSERT_EQ(func->ret(), entt::resolve<void>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(func->arg(entt::meta_func::size_type{1}), nullptr);

    auto any = func->invoke({}, 42);

    ASSERT_FALSE(any);
    ASSERT_EQ(func_type::value, 42);

    func->prop([](auto *prop) {
        ASSERT_NE(prop, nullptr);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_EQ(func->prop(properties::prop_int), nullptr);

    auto *prop = func->prop(properties::prop_bool);

    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->key(), properties::prop_bool);
    ASSERT_FALSE(prop->value().cast<bool>());
}

TEST_F(Meta, MetaFuncMetaAnyArgs) {
    // TODO
}

TEST_F(Meta, MetaFuncInvalidArgs) {
    // TODO
}

TEST_F(Meta, MetaFuncCastAndConvert) {
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

TEST_F(Meta, DataFromBase) {
    // TODO
}

TEST_F(Meta, FuncFromBase) {
    // TODO
}
