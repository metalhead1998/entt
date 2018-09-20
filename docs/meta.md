# Crash Course: meta

<!--
@cond TURN_OFF_DOXYGEN
-->
# Table of Contents

TODO to be updated

* [Introduction](#introduction)
* [Reflection in a nutshell](#reflection-in-a-nutshell)
* [Any as in any type](#any-as-in-any-type)
* [Enjoy the runtime](#enjoy-the-runtime)
* [Properties and meta objects](#properties-and-meta-objects)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

Reflection (or rather, its lack) is a trending topic in the C++ world and, in
the specific case of `EnTT`, a tool that can unlock a lot of other features. I
looked for a third-party library that met my needs on the subject, but I always
came across some details that I didn't like: macros, being intrusive, too many
allocations. In one word: unsatisfactory.<br/>
I finally decided to write a, non-intrusive and macro-free runtime built-in
reflection system for `EnTT. Maybe I didn't do better than others or maybe yes,
time will tell me, but at least I can model this tool around the library to
which it belongs and not vice versa.

# Reflection in a nutshell

Reflection always starts from real types (users cannot reflect imaginary types
and it would not make much sense, we wouldn't be talking about reflection
anymore).<br/>
To _reflect_ a type, the library provides the `reflect` function:

```cpp
auto factory = entt::reflect<my_type>("reflected_type");
```

It accepts the type to reflect as a template parameter and the name to give it
once reflected as an argument. Names are important because users will be able to
access meta types at runtime by searching for them by name. The returned value
is a factory object to use to continue building the meta type.

Let's consider now the following type and declarations:

```cpp
struct my_type {
    my_type(int);

    int value;
    static double cvalue;

    void func();
    static void cfunc();
};

my_type my_ctor(const char *);
void my_dtor(my_type &);
```

Actual constructors can be assigned to a meta type by specifying their list of
arguments. As a constructor, free functions can also be used, as well as for
destructors. From a client's point of view, nothing changes if constructors and
destructors are free functions or actual constructors and destructors.<br/>
Starting from the previously defined type, we therefore have the following:

```cpp
entt::reflect<my_type>("reflected_type")
    .ctor<int>()
    .ctor<&my_ctor>()
    .dtor<&my_dtor>();
```

Meta data of a meta type can be both real data members of the underlying type
and static or global variables. From a client's point of view, all the variables
associated with the reflected object will appear as if they were part of the
type.<br/>
Starting from the previously defined type, we therefore have the following:

```cpp
entt::reflect<my_type>("reflected_type")
    .ctor<int>()
    .ctor<&my_ctor>()
    .dtor<&my_dtor>()
    .data<&my_type::value>("value")
    .data<&my_type::cvalue>("cvalue");
```

The `data` member function of a factory accepts the name to give to the meta
data once created as an argument. Names are important because users will be able
to access meta data at runtime by searching for them by name.

Meta functions of a meta type can be both real member functions of the
underlying type and free functions. From a client's point of view, all the
functions associated with the reflected object will appear as if they were part
of the type.<br/>
Starting from the previously defined type, we therefore have the following:

```cpp
entt::reflect<my_type>("reflected_type")
    .ctor<int>()
    .ctor<&my_ctor>()
    .dtor<&my_dtor>()
    .data<&my_type::value>("value")
    .data<&my_type::cvalue>("cvalue")
    .func<&my_type::func>("func")
    .func<&my_type::cfunc>("cfunc");
```

The `func` member function of a factory accepts the name to give to the meta
function once created as an argument. Names are important because users will be
able to access meta functions at runtime by searching for them by name.

That's all, everything users need to create meta types and enjoy the reflection
system. At first glance it may not seem that much, but users usually learn to
appreciate it over time.<br/>
Also, do not forget what these few lines hide under the hood: a non-intrusive
and macro-free built-in runtime system for reflection in C++. Features that are
definitely worth the price, at least for me.

# Any as in any type

The reflection system comes with its own meta any type. It may seem redundant
since C++17 introduced `std::any`, but it is not.<br/>
In fact, the _type_ returned by an `std::any` is a const reference to an
`std::type_info`, an implementation defined class that's not something everyone
wants to see in their software. Furthermore, the class `std::type_info` suffers
from some design flaws and there is no way to _convert_ an `std::type_info` into
a meta type, thus linking the two worlds.

A meta any object provides an API similar to that of its most famous counterpart
and serves the same purpose of being an opaque container for any type of
value.<br/>
The `type` member function returns a pointer to the meta type of the contained
value, if any. On the other side, the `to` and the `data` member functions can
be used to retrieve either a reference or a pointer to that value. Some other
functions to know if the object can be converted to a given type and, therefore,
to implicitly convert it, complete the offer of this class.<br/>
Refer to the inline documentation for all the details.

# Enjoy the runtime

Once the web of reflected types has been constructed, it's a matter of using it
at runtime where required.<br/>
All this has the great merit that, unlike the vast majority of the things
present in this library and closely linked to the compile-time, the reflection
system stands in fact as a non-intrusive tool for the runtime.

To search for a reflected type there are two options: by type or by name. In
both cases, the search can be done by means of the `resolve` function:

```cpp
// search for a reflected type by type
auto *by_type = entt::resolve<my_type>();

// search for a reflected type by name
auto *by_name = entt::resolve("reflected_type");
```

The returned value is a pointer to a `meta_type` object. This type of objects
offer a series of useful tools to know the name, to iterate all the meta
objects associated with them and even to build or destroy instances of the
underlying type.<br/>
Refer to the inline documentation for all the details.

Once someone has a meta-type, meta data and meta functions can be easily
retrieved by means of two member functions: `data` and `func`. In both cases,
members are searched by name:

```cpp
auto *data = entt::resolve<my_type>().data("cvalue")
auto *func = entt::resolve<my_type>().func("cfunc")
```

Meta data offer a set of member functions including methods to know the name, to
get the meta type of the underlying variable, to set and get the value of the
actual variable and so on.<br/>
Meta functions offer a slightly different set of member functions, among them
methods to know the meta type associated to the returnr type, to retrieve the
meta types of the arguments, to invoke the underlying function and so on.<br/>
Refer to the inline documentation for all the details.

Similarly, constructors can also be retrieved (by argument list) as well as the
destructor associated with the meta type. As for the other meta objects, also in
this case we have a set of tools to invoke and explore constructors and
destructors in all their parts.

Finally, meta types and meta objects in general contain much more than what is
said: a plethora of functions in addition to those listed whose purposes and
uses go unfortunately beyond the scope of this document.<br/>
I invite anyone interested in the subject to look at the code, experiment and
read the official documentation to get the best out of this powerful tool.

# Properties and meta objects

Sometimes (ie when it comes to creating an editor) it might be useful to be able
to attach properties to the meta objects created. Fortunately, this is possible
for all of them.<br/>
To attach a property to a meta object, no matter what, it is sufficient to
provide an object at the time of construction such that `std::get<0>` and
`std::get<1>` are valid for it. In other terms, the properties are nothing more
than key/value pairs users can put in an `std::pair`. As an example:

```cpp
entt::reflect<my_type>("reflected_type", std::make_pair("tooltip"_hs, "message"));
```

All the meta objects offer then a couple of member functions named `prop` to
iterate all the properties at once and to search a specific property by key:

```cpp
// iterate all the properties of a meta type
entt::resolve<my_type>().prop([](auto *prop) { /* ... */ });

// search for a given property by name
auto *prop = entt::resolve<my_type>().prop("tooltip"_hs);
```

Meta properties are objects having a fairly poor interface, all in all. They
only provide the `key` and the `value` member functions to be used to retrieve
the key and the value contained in the form of meta any objects, respectively.
