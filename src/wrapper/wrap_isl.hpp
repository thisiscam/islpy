#include "wrap_helpers.hpp"
#include <isl/ctx.h>
#include <isl/id.h>
#include <isl/space.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/union_set.h>
#include <isl/union_map.h>
#include <isl/point.h>
#include <isl/printer.h>
#include <isl/local_space.h>
#include <isl/vec.h>
#include <isl/mat.h>
#include <isl/polynomial.h>
#include <isl/aff.h>
#include <isl/aff.h>
#include <isl/ilp.h>
#include <isl/vertices.h>
#include <isl/schedule.h>
#if !defined(ISLPY_ISL_VERSION) || (ISLPY_ISL_VERSION >= 15)
#include <isl/schedule_node.h>
#endif
#include <isl/flow.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/options.h>

#ifdef ISLPY_INCLUDE_BARVINOK
#include <barvinok/isl.h>
#endif

#include <iostream>
#include <stdexcept>
#include <pybind11/pybind11.h>


// TODO: flow.h
// TODO: better error reporting

namespace py = pybind11;

namespace isl
{
  class error : public std::runtime_error
  {
    public:
      explicit error (const std::string &what)
        : std::runtime_error(what)
      { }
  };

  struct ctx;

  typedef std::unordered_map<isl_ctx *, unsigned> ctx_use_map_t;
  extern ctx_use_map_t ctx_use_map;

  inline void ref_ctx(isl_ctx *data)
  {
    ctx_use_map_t::iterator it(ctx_use_map.find(data));
    if (it == ctx_use_map.end())
      ctx_use_map[data] = 1;
    else
      ctx_use_map[data] += 1;
  }

  inline void deref_ctx(isl_ctx *ctx)
  {
    ctx_use_map[ctx] -= 1;
    if (ctx_use_map[ctx] == 0)
      isl_ctx_free(ctx);
  }

#define WRAP_CLASS(name) \
  struct name { WRAP_CLASS_CONTENT(name) }

#define MAKE_CAST_CTOR(name, from_type, cast_func) \
      name(from_type &data) \
      : m_valid(false) \
      { \
        m_ctx = isl_##from_type##_get_ctx(data.m_data); \
        \
        isl_##from_type *copy = isl_##from_type##_copy(data.m_data); \
        if (!copy) \
          throw error("isl_" #from_type "_copy failed"); \
        m_data = cast_func(copy); \
        if (!m_data) \
          throw error(#cast_func " failed"); \
        \
        m_valid = true; \
        ref_ctx(m_ctx); \
      }

#define WRAP_CLASS_CONTENT(name) \
    private: \
      bool              m_valid; \
      isl_ctx           *m_ctx; \
    public: \
      isl_##name        *m_data; \
      \
      name(isl_##name *data) \
      : m_valid(false), m_data(nullptr) \
      /* passing nullptr is allowed to create a (temporarily invalid) */ \
      /* instance during unpickling */ \
      { \
        take_possession_of(data); \
      } \
      \
      void invalidate() \
      { \
        if (m_valid) \
        { \
          deref_ctx(m_ctx); \
          m_valid = false; \
        } \
      } \
      \
      bool is_valid() const \
      { \
        return m_valid; \
      } \
      \
      ~name() \
      { \
        free_instance(); \
      } \
      \
      void free_instance() \
      { \
        if (m_valid) \
        { \
          isl_##name##_free(m_data); \
          deref_ctx(m_ctx); \
          m_valid = false; \
        } \
      } \
      \
      void take_possession_of(isl_##name *data) \
      { \
        free_instance(); \
        if (data) \
        { \
          m_data = data; \
          m_valid = true; \
          m_ctx = isl_##name##_get_ctx(data); \
          ref_ctx(m_ctx); \
        } \
      } \
      \
      void steal_instance(name &other) \
      { \
        take_possession_of(other.m_data); \
        other.invalidate(); \
      }

  struct ctx \
  {
    public:
      isl_ctx           *m_data;

      ctx(isl_ctx *data)
      : m_data(data)
      {
        ref_ctx(data);
      }

      bool is_valid() const
      {
        return true;
      }

      ~ctx()
      {
        deref_ctx(m_data);
      }

      void reset_instance(ctx &other)
      {
        ref_ctx(other.m_data);
        deref_ctx(m_data);
        m_data = other.m_data;
      }

      bool wraps_same_instance_as(ctx const &other)
      {
        return m_data == other.m_data;
      }
  };

  // matches order in gen_wrap.py

  // {{{ part 1

  WRAP_CLASS(id_list);
  WRAP_CLASS(val_list);
  WRAP_CLASS(basic_set_list);
  WRAP_CLASS(basic_map_list);
  WRAP_CLASS(set_list);
  WRAP_CLASS(map_list);
  WRAP_CLASS(union_set_list);
  WRAP_CLASS(constraint_list);
  WRAP_CLASS(aff_list);
  WRAP_CLASS(pw_aff_list);
  WRAP_CLASS(pw_multi_aff_list);
  WRAP_CLASS(ast_expr_list);
  WRAP_CLASS(ast_node_list);
  WRAP_CLASS(pw_qpolynomial_list);
  WRAP_CLASS(pw_qpolynomial_fold_list);
  WRAP_CLASS(union_pw_aff_list);
  WRAP_CLASS(union_pw_multi_aff_list);
  WRAP_CLASS(union_map_list);

  WRAP_CLASS(id_to_ast_expr);

  WRAP_CLASS(printer);
  WRAP_CLASS(val);
  WRAP_CLASS(multi_val);
  WRAP_CLASS(vec);
  WRAP_CLASS(mat);
  WRAP_CLASS(fixed_box);

  WRAP_CLASS(aff);
  struct pw_aff
  {
    WRAP_CLASS_CONTENT(pw_aff);
    MAKE_CAST_CTOR(pw_aff, aff, isl_pw_aff_from_aff);
  };
  WRAP_CLASS(union_pw_aff);
  WRAP_CLASS(multi_aff);
  WRAP_CLASS(multi_pw_aff);
  WRAP_CLASS(pw_multi_aff);
  WRAP_CLASS(union_pw_multi_aff);
  WRAP_CLASS(multi_union_pw_aff);

  WRAP_CLASS(id);
  WRAP_CLASS(multi_id);

  WRAP_CLASS(constraint);
  WRAP_CLASS(space);
  struct local_space
  {
    WRAP_CLASS_CONTENT(local_space);
    MAKE_CAST_CTOR(local_space, space, isl_local_space_from_space);
  };

  // }}}

  // {{{ part 2

  WRAP_CLASS(basic_set);
  WRAP_CLASS(basic_map);

  struct set
  {
    WRAP_CLASS_CONTENT(set);
    MAKE_CAST_CTOR(set, basic_set, isl_set_from_basic_set);
  };

  struct map
  {
    WRAP_CLASS_CONTENT(map);
    MAKE_CAST_CTOR(map, basic_map, isl_map_from_basic_map);
  };

  struct union_set
  {
    WRAP_CLASS_CONTENT(union_set);
    MAKE_CAST_CTOR(union_set, set, isl_union_set_from_set);
  };
  struct union_map
  {
    WRAP_CLASS_CONTENT(union_map);
    MAKE_CAST_CTOR(union_map, map, isl_union_map_from_map);
  };

  WRAP_CLASS(point);
  WRAP_CLASS(vertex);
  WRAP_CLASS(cell);
  WRAP_CLASS(vertices);
  WRAP_CLASS(stride_info);

  // }}}

  // {{{ part 3

  WRAP_CLASS(qpolynomial);
  WRAP_CLASS(pw_qpolynomial);
  WRAP_CLASS(qpolynomial_fold);
  WRAP_CLASS(pw_qpolynomial_fold);
  WRAP_CLASS(union_pw_qpolynomial);
  WRAP_CLASS(union_pw_qpolynomial_fold);
  WRAP_CLASS(term);

  WRAP_CLASS(schedule);
  WRAP_CLASS(schedule_constraints);
  WRAP_CLASS(schedule_node);

  WRAP_CLASS(access_info);
  WRAP_CLASS(flow);
  WRAP_CLASS(restriction);
  WRAP_CLASS(union_access_info);
  WRAP_CLASS(union_flow);

  WRAP_CLASS(ast_expr);
  WRAP_CLASS(ast_node);
  WRAP_CLASS(ast_print_options);
  WRAP_CLASS(ast_build);

  // }}}

  class format { };
  class yaml_style { };
  class bound { };
  class on_error { };
  class schedule_algorithm { };

  inline void my_decref(void *user)
  {
    Py_DECREF((PyObject *) user);
  }
}





#define MAKE_WRAP(name, py_name) \
  py::class_<isl::name> wrap_##name(m, #py_name, py::dynamic_attr()); \
  wrap_##name.def("_is_valid", &isl::name::is_valid); \
  wrap_##name.attr("_base_name") = #name; \
  wrap_##name.attr("_isl_name") = "isl_"#name; \
  wrap_##name.def("_steal_instance", &isl::name::steal_instance); \

#define WRAP_ENABLE_PICKLING(name) \
  wrap_##name.def(py::pickle( \
        [](isl::name const &p) /* __getstate__ */ \
        { \
          throw isl::error("__getstate__ called for islpy object"); \
          return py::none(); \
        }, \
        [](py::none obj) /* __setstate__ */ \
        { \
          return isl::name(nullptr); \
        } \
        ))

// vim: foldmethod=marker
