#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "constructors_mixin.h"
#include "constructors_simple2.h"

#define MY_CLASS SIMPLE2_CLASS

static Eo *
_constructor(Eo *obj, void *class_data EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   return NULL;
}

static Efl_Op_Description op_descs[] = {
     EFL_OBJECT_OP_FUNC_OVERRIDE(efl_constructor, _constructor),
};

static const Efl_Class_Description class_desc = {
     EO_VERSION,
     "Simple2",
     EFL_CLASS_TYPE_REGULAR,
     EFL_CLASS_DESCRIPTION_OPS(op_descs),
     0,
     NULL,
     NULL
};

EFL_DEFINE_CLASS(simple2_class_get, &class_desc, EO_CLASS, NULL);

