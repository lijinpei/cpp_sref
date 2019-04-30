# Cpp Static Reflection

Cpp_sref is a tool to do compile time reflection with the help of clang ast.

## Api

All the constructs provided by this tool lives in the static_reflection namespace.

```C++
namespace static_reflection {
}
```

### Reflection Class
For any class that lives outsides of static_reflection, a corresponding reflection class lives inside the static_reflection namespacewith similiar path, those classes that live inside static_reflection namespace can not be reflected.

A reflection class has the following members:
```C++
namespace static_reflection {
  enum type_kind {
    tk_primitive,
    tk_pointer,
    tk_array,
    tk_function,
    tk_enum,
    tk_class,
  };

  concept reflection_name {
    static constexpr std::size_t length;
    static constexpr char name[length];
  };

  concept reflection_type {
    static constexpr type_kind kind;
    /* for closure type, don't count on too much on its name */
    static constexpr reflection_name name;
    typename reflected_type;
    static constexpr std::size_t align;
    static constexpr std::size_t size;
    static constexpr bool is_const;
    static constexpr bool is_volatile;

    /* if (kind == tk_primitive) { */
    static constexpr bool is_integral;
    static constexpr bool is_arithmetic;
    static constexpr bool is_signed;
    /* it is not necessary that !is_signed == is_unsigned */
    static constexpr bool is_unsigned;

    /* } else if (kind == tk_pointer) { */
    typename pointee_type;
    /* } else if (kind == tk_array) { */
    typename element_type;
    static constexpr std::size_t num_elements;
    /* } else if (kind == tk_function) { */
    static constexpr std::size_t arity;
    typename return_type;
    typename function_parameter_list = list<reflection_function_parameter>; // a list of parameters
    /* } else if (kind == tk_enum) { */
    /* } else if (kind == tk_class) { */
    typename base_list = list<reflection_base_class>; // a list of bases
    typename member_list = list<reflection_member>; // a list of members
    typename method_list = list<reflection_method>; // a list of methods
    /* } */
  };

  concept reflection_function {
  };

  concept reflection_base_class: reflection_class {
    static constexpr bool is_private;
    static constexpr bool is_protected;
    static constexpr bool is_public;
    static constexpr bool is_virtual;
  };

  concept reflection_member: reflection_function {
    static constexpr bool this_const;
    static constexpr bool this_volatile;
    static constexpr bool this_lvalue_ref;
    static constexpr bool this_rvalue_ref;
  };
}
```
