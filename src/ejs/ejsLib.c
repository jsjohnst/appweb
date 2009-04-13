#include "ejs.h"

/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Embedthis Ejscript 0.9.8.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify ejs, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../types/core/ejsArray.c"
 */
/************************************************************************/

/**
 *  ejsArray.c - Ejscript Array class
 *
 *  This module implents the standard Array type. It provides the type methods and manages the special "length" property.
 *  The array elements with numeric indicies are stored in EjsArray.data[]. Non-numeric properties are stored in EjsArray.obj.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int  checkSlot(Ejs *ejs, EjsArray *ap, int slotNum);
static bool compare(Ejs *ejs, EjsVar *v1, EjsVar *v2);
static int growArray(Ejs *ejs, EjsArray *ap, int len);
static int lookupArrayProperty(Ejs *ejs, EjsArray *ap, EjsName *qname);
static EjsVar *pushArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv);
static EjsVar *spliceArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv);
static EjsVar *arrayToString(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv);

#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
static EjsVar *makeIntersection(Ejs *ejs, EjsArray *lhs, EjsArray *rhs);
static EjsVar *makeUnion(Ejs *ejs, EjsArray *lhs, EjsArray *rhs);
static EjsVar *removeArrayElements(Ejs *ejs, EjsArray *lhs, EjsArray *rhs);
#endif

/*
 *  Cast the object operand to a primitive type
 */

static EjsVar *castArray(Ejs *ejs, EjsArray *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejs->zeroValue;

    case ES_String:
        return arrayToString(ejs, vp, 0, 0);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


static EjsArray *cloneArray(Ejs *ejs, EjsArray *ap, bool deep)
{
    EjsArray    *newArray;
    EjsVar      **dest, **src;
    int         i;

    newArray = (EjsArray*) ejsCopyObject(ejs, (EjsObject*) ap, deep);
    if (newArray == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    if (ap->length > 0) {
        if (growArray(ejs, newArray, ap->length) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
        
        src = ap->data;
        dest = newArray->data;

        if (deep) {
            for (i = 0; i < ap->length; i++) {
                dest[i] = ejsCloneVar(ejs, src[i], 1);
                ejsSetReference(ejs, (EjsVar*) ap, dest[i]);
            }

        } else {
            memcpy(dest, src, ap->length * sizeof(EjsVar*));
        }
    }
    return newArray;
}


/*
 *  Delete a property and update the length
 */
static int deleteArrayProperty(Ejs *ejs, EjsArray *ap, int slot)
{
    if (slot >= ap->length) {
        //  TODO DIAG
        mprAssert(0);
        return EJS_ERR;
    }
    if ((slot + 1) == ap->length) {
        ap->length--;
    }

    if (ejsSetProperty(ejs, (EjsVar*) ap, slot, (EjsVar*) ejs->undefinedValue) < 0) {
        //  TODO - DIAG
        return EJS_ERR;
    }

    return 0;
}


/*
 *  Delete an element by name.
 */
static int deleteArrayPropertyByName(Ejs *ejs, EjsArray *ap, EjsName *qname)
{
    if (isdigit((int) qname->name[0])) {
        return deleteArrayProperty(ejs, ap, atoi(qname->name));
    }

    return (ejs->objectHelpers->deletePropertyByName)(ejs, (EjsVar*) ap, qname);
}


/*
 *  Return the number of elements in the array
 */
static int getArrayPropertyCount(Ejs *ejs, EjsArray *ap)
{
    return ap->length;
}


/*
 *  Get an array element. Slot numbers correspond to indicies.
 */
static EjsVar *getArrayProperty(Ejs *ejs, EjsArray *ap, int slotNum)
{
    if (slotNum < 0 || slotNum >= ap->length) {
        return ejs->undefinedValue;
#if UNUSED
        ejsThrowOutOfBoundsError(ejs, "Bad array subscript");
        return 0;
#endif
    }
    return ap->data[slotNum];
}


static EjsVar *getArrayPropertyByName(Ejs *ejs, EjsArray *ap, EjsName *qname)
{
    int     slotNum;

    if (isdigit((int) qname->name[0])) { 
        slotNum = atoi(qname->name);
        if (slotNum < 0 || slotNum >= ap->length) {
            return 0;
        }
        return getArrayProperty(ejs, ap, slotNum);
    }

    /* The "length" property is a method getter */
    if (strcmp(qname->name, "length") == 0) {
        return 0;
    }
    slotNum = (ejs->objectHelpers->lookupProperty)(ejs, (EjsVar*) ap, qname);
    if (slotNum < 0) {
        return 0;
    }
    return (ejs->objectHelpers->getProperty)(ejs, (EjsVar*) ap, slotNum);
}


/*
 *  Lookup an array index.
 */
static int lookupArrayProperty(Ejs *ejs, EjsArray *ap, EjsName *qname)
{
    int     index;

    if (qname == 0 || !isdigit((int) qname->name[0])) {
        return EJS_ERR;
    }
    index = atoi(qname->name);
    if (index < ap->length) {
        return index;
    }

    return EJS_ERR;
}


/*
 *  Cast operands as required for invokeArrayOperator
 */
static EjsVar *coerceArrayOperands(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, arrayToString(ejs, (EjsArray*) lhs, 0, 0), opcode, rhs);

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
        if (ejsIsNull(rhs) || ejsIsUndefined(rhs)) {
            return (EjsVar*) ((opcode == EJS_OP_COMPARE_EQ) ? ejs->falseValue: ejs->trueValue);
        } else if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }

    return 0;
}


static EjsVar *invokeArrayOperator(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceArrayOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, (lhs == rhs));

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs == rhs));

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->oneValue;

    /*
     *  Binary operators
     */
    case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_REM:
    case EJS_OP_SHR: case EJS_OP_USHR: case EJS_OP_XOR:
        return (EjsVar*) ejs->zeroValue;

#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
    /*
     *  Operator overload
     */
    case EJS_OP_ADD:
        result = (EjsVar*) ejsCreateArray(ejs, 0);
        pushArray(ejs, (EjsArray*) result, 1, (EjsVar**) &lhs);
        pushArray(ejs, (EjsArray*) result, 1, (EjsVar**) &rhs);
        return result;

    case EJS_OP_AND:
        return (EjsVar*) makeIntersection(ejs, (EjsArray*) lhs, (EjsArray*) rhs);

    case EJS_OP_OR:
        return (EjsVar*) makeUnion(ejs, (EjsArray*) lhs, (EjsArray*) rhs);

    case EJS_OP_SHL:
        return pushArray(ejs, (EjsArray*) lhs, 1, &rhs);

    case EJS_OP_SUB:
        return (EjsVar*) removeArrayElements(ejs, (EjsArray*) lhs, (EjsArray*) rhs);
#endif

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


static void markArrayVar(Ejs *ejs, EjsVar *parent, EjsArray *ap)
{
    EjsVar          *vp;
    int             i;

    mprAssert(ejsIsArray(ap));

    ejsMarkObject(ejs, parent, (EjsObject*) ap);
    
    for (i = ap->length - 1; i >= 0; i--) {
        if ((vp = ap->data[i]) == 0) {
            continue;
        }
        ejsMarkVar(ejs, (EjsVar*) ap, vp);
    }
}


/*
 *  Create or update an array elements. If slotNum is < 0, then create the next free array slot. If slotNum is greater
 *  than the array length, grow the array.
 */
static int setArrayProperty(Ejs *ejs, EjsArray *ap, int slotNum,  EjsVar *value)
{
    if ((slotNum = checkSlot(ejs, ap, slotNum)) < 0) {
        return EJS_ERR;
    }
    ap->data[slotNum] = value;
    ejsSetReference(ejs, (EjsVar*) ap, value);
    return slotNum;
}


static int setArrayPropertyByName(Ejs *ejs, EjsArray *ap, EjsName *qname, EjsVar *value)
{
    int     slotNum;

    if (!isdigit((int) qname->name[0])) { 
        /* The "length" property is a method getter */
        if (strcmp(qname->name, "length") == 0) {
            return EJS_ERR;
        }
        slotNum = (ejs->objectHelpers->lookupProperty)(ejs, (EjsVar*) ap, qname);
        if (slotNum < 0) {
            slotNum = (ejs->objectHelpers->setProperty)(ejs, (EjsVar*) ap, slotNum, value);
            if (slotNum < 0) {
                return EJS_ERR;
            }
            if ((ejs->objectHelpers->setPropertyName)(ejs, (EjsVar*) ap, slotNum, qname) < 0) {
                return EJS_ERR;
            }
            return slotNum;

        } else {
            return (ejs->objectHelpers->setProperty)(ejs, (EjsVar*) ap, slotNum, value);
        }
    }

    if ((slotNum = checkSlot(ejs, ap, atoi(qname->name))) < 0) {
        return EJS_ERR;
    }
    ap->data[slotNum] = value;
    ejsSetReference(ejs, (EjsVar*) ap, value);

    return slotNum;
}


#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
static EjsVar *makeIntersection(Ejs *ejs, EjsArray *lhs, EjsArray *rhs)
{
    EjsArray    *result;
    EjsVar      **l, **r, **resultSlots;
    int         i, j, k;

    result = ejsCreateArray(ejs, 0);
    l = lhs->data;
    r = rhs->data;

    for (i = 0; i < lhs->length; i++) {
        for (j = 0; j < rhs->length; j++) {
            if (compare(ejs, l[i], r[j])) {
                resultSlots = result->data;
                for (k = 0; k < result->length; k++) {
                    if (compare(ejs, l[i], resultSlots[k])) {
                        break;
                    }
                }
                if (result->length == 0 || k == result->length) {
                    setArrayProperty(ejs, result, -1, l[i]);
                }
            }
        }
    }
    return (EjsVar*) result;
}


static int addUnique(Ejs *ejs, EjsArray *ap, EjsVar *element)
{
    int     i;

    for (i = 0; i < ap->length; i++) {
        if (compare(ejs, ap->data[i], element)) {
            break;
        }
    }
    if (i == ap->length) {
        if (setArrayProperty(ejs, ap, -1, element) < 0) {
            return EJS_ERR;
        }
    }
    return 0;
}


static EjsVar *makeUnion(Ejs *ejs, EjsArray *lhs, EjsArray *rhs)
{
    EjsArray    *result;
    EjsVar      **l, **r;
    int         i;

    result = ejsCreateArray(ejs, 0);

    l = lhs->data;
    r = rhs->data;

    for (i = 0; i < lhs->length; i++) {
        addUnique(ejs, result, l[i]);
    }
    for (i = 0; i < rhs->length; i++) {
        addUnique(ejs, result, r[i]);
    }

    return (EjsVar*) result;
}


static EjsVar *removeArrayElements(Ejs *ejs, EjsArray *lhs, EjsArray *rhs)
{
    EjsVar  **l, **r;
    int     i, j, k;

    l = lhs->data;
    r = rhs->data;

    for (j = 0; j < rhs->length; j++) {
        for (i = 0; i < lhs->length; i++) {
            if (compare(ejs, l[i], r[j])) {
                for (k = i + 1; k < lhs->length; k++) {
                    l[k - 1] = l[k];
                }
                lhs->length--;
            }
        }
    }

    return (EjsVar*) lhs;
}
#endif


static int checkSlot(Ejs *ejs, EjsArray *ap, int slotNum)
{
    if (slotNum < 0) {
        if (!ap->obj.var.dynamic) {
            ejsThrowTypeError(ejs, "Object is not dynamic");
            return EJS_ERR;
        }
        
        slotNum = ap->length;
        if (growArray(ejs, ap, ap->length + 1) < 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }

    } else if (slotNum >= ap->length) {
        if (growArray(ejs, ap, slotNum + 1) < 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }
    }
    return slotNum;
}


/*
 *  Array constructor.
 *
 *  function Array(...args): Array
 *
 *  Support the forms:
 *
 *      var arr = Array();
 *      var arr = Array(size);
 *      var arr = Array(elt, elt, elt, ...);
 */

static EjsVar *arrayConstructor(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      *arg0, **src, **dest;
    int         size, i;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];
    
    if (args->length == 0) {
        return 0;
    }

    size = 0;
    arg0 = getArrayProperty(ejs, args, 0);

    if (args->length == 1 && ejsIsNumber(arg0)) {
        /*
         *  x = new Array(size);
         */
        size = ejsGetInt(arg0);
        if (size > 0 && growArray(ejs, ap, size) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }

    } else {

        /*
         *  x = new Array(element0, element1, ..., elementN):
         */
        size = args->length;
        if (size > 0 && growArray(ejs, ap, size) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }

        src = args->data;
        dest = ap->data;
        for (i = 0; i < size; i++) {
            dest[i] = src[i];
        }
    }
    ap->length = size;

    return (EjsVar*) ap;
}


/*
 *  Append an item to an array
 *
 *  function append(obj: Object) : Array
 */
static EjsVar *appendArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    if (setArrayProperty(ejs, ap, ap->length, argv[0]) < 0) {
        return 0;
    }
    return (EjsVar*) ap;
}


/*
 *  Clear an array. Remove all elements of the array.
 *
 *  function clear() : void
 */
static EjsVar *clearArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    ap->length = 0;
    return 0;
}


/*
 *  Clone an array.
 *
 *  function clone(deep: Boolean = false) : Array
 */
static EjsArray *cloneArrayMethod(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    bool    deep;

    mprAssert(argc == 0 || ejsIsBoolean(argv[0]));

    deep = (argc == 1) ? ((EjsBoolean*) argv[0])->value : 0;

    return cloneArray(ejs, ap, deep);
}


/*
 *  Compact an array. Remove all null elements.
 *
 *  function compact() : Array
 */
static EjsArray *compactArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      **data, **src, **dest;
    int         i;

    data = ap->data;
    src = dest = &data[0];
    for (i = 0; i < ap->length; i++, src++) {
        if (*src == 0 || *src == ejs->undefinedValue || *src == ejs->nullValue) {
            continue;
        }
        *dest++ = *src;
    }

    ap->length = (int) (dest - &data[0]);
    return ap;
}


/*
 *  Concatenate the supplied elements with the array to create a new array. If any arguments specify an array,
 *  their elements are catenated. This is a one level deep copy.
 *
 *  function concat(...args): Array
 */
static EjsVar *concatArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args, *newArray, *vpa;
    EjsVar      *vp, **src, **dest;
    int         i, k, next;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = ((EjsArray*) argv[0]);

    /*
     *  Guess the new array size. May exceed this if args has elements that are themselves arrays.
     */
    newArray = ejsCreateArray(ejs, ap->length + ((EjsArray*) argv[0])->length);

    src = ap->data;
    dest = newArray->data;

    /*
     *  Copy the original array
     */
    for (next = 0; next < ap->length; next++) {
        dest[next] = src[next];
    }

    /*
     *  Copy the args. If any element is itself an array, then flatten it and copy its elements.
     */
    for (i = 0; i < args->length; i++) {
        vp = args->data[i];
        if (ejsIsArray(vp)) {

            vpa = (EjsArray*) vp;
            if (growArray(ejs, newArray, newArray->length + vpa->length - 1) < 0) {
                ejsThrowMemoryError(ejs);
                return 0;
            }
            for (k = 0; k < vpa->length; k++) {
                dest[next++] = src[k];
            }

        } else {
            dest[next++] = vp;
        }
    }

    return 0;
}


/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextArrayKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsArray        *ap;
    EjsVar          *vp, **data;

    ap = (EjsArray*) ip->target;
    if (!ejsIsArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }
    data = ap->data;

    for (; ip->index < ap->length; ip->index++) {
        vp = data[ip->index];
        if (vp == 0) {
            continue;
        }
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getArrayIterator(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextArrayKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextArrayValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsArray    *ap;
    EjsVar      *vp, **data;

    ap = (EjsArray*) ip->target;
    if (!ejsIsArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    data = ap->data;
    for (; ip->index < ap->length; ip->index++) {
        vp = data[ip->index];
        if (vp == 0) {
            continue;
        }
        ip->index++;
        return vp;
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getArrayValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextArrayValue, 0, NULL);
}


#if UNUSED && KEEP
static EjsVar *find(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    return 0;
}


/**
 *  Iterate over all elements in the object and find all elements for which the matching function is true.
 *  The match is called with the following signature:
 *
 *      function match(arrayElement: Object, elementIndex: Number, arr: Array): Boolean
 *
 *  @param match Matching function
 *  @return Returns a new array containing all matching elements.
 */
static EjsVar *findAll(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      *funArgs[3];
    EjsBoolean  *result;
    EjsArray    *elements;
    int         i;

    mprAssert(argc == 1 && ejsIsFunction(argv[0]));

    elements = ejsCreateArray(ejs, 0);
    if (elements == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    for (i = 0; i < ap->length; i++) {
        funArgs[0] = ap->obj.properties.slots[i];               /* Array element */
        funArgs[1] = (EjsVar*) ejsCreateNumber(ejs, i);             /* element index */
        funArgs[2] = (EjsVar*) ap;                                  /* Array */
        result = (EjsBoolean*) ejsRunFunction(ejs, (EjsFunction*) argv[0], 0, 3, funArgs);
        if (result == 0 || !ejsIsBoolean(result) || !result->value) {
            setArrayProperty(ejs, elements, elements->length, ap->obj.properties.slots[i]);
        }
    }
    return (EjsVar*) elements;
}
#endif


//  TODO - this routine should be somewhere common
static bool compare(Ejs *ejs, EjsVar *v1, EjsVar *v2)
{
    if (v1 == v2) {
        return 1;
    }
    if (v1->type != v2->type) {
        return 0;
    }
    if (ejsIsNumber(v1)) {
        return ((EjsNumber*) v1)->value == ((EjsNumber*) v2)->value;
    }
    if (ejsIsString(v1)) {
        return strcmp(((EjsString*) v1)->value, ((EjsString*) v2)->value) == 0;
    }
    //  TODO - is this right?
    return 0;
}


/*
 *  Search for an item using strict equality "===". This call searches from
 *  the start of the array for the specified element.
 *  @return Returns the items index into the array if found, otherwise -1.
 *
 *  function indexOf(element: Object, startIndex: Number = 0): Number
 */
static EjsVar *indexOfArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar          *element;
    int             i, start;

    mprAssert(argc == 1 || argc == 2);

    element = argv[0];
    start = (argc == 2) ? (int) ((EjsNumber*) argv[1])->value : 0;

    for (i = start; i < ap->length; i++) {
        if (compare(ejs, ap->data[i], element)) {
            return (EjsVar*) ejsCreateNumber(ejs, i);
        }
    }
    return (EjsVar*) ejs->minusOneValue;
}


/*
 *  Insert elements. Insert elements at the specified position. Negative indicies are measured from the end of the array.
 *  @return Returns a the original array.
 *
 *  function insert(pos: Number, ...args): Array
 */
static EjsVar *insertArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      **src, **dest;
    int         i, pos, delta, oldLen, endInsert;

    mprAssert(argc == 2 && ejsIsArray(argv[1]));

    pos = ejsGetInt(argv[0]);
    if (pos < 0) {
        pos += ap->length;
    }
    args = (EjsArray*) argv[1];

    oldLen = ap->length;
    if (growArray(ejs, ap, ap->length + args->length) < 0) {
        return 0;
    }

    delta = args->length;
    dest = ap->data;
    src = args->data;

    endInsert = pos + delta;
    for (i = ap->length - 1; i >= endInsert; i--) {
        dest[i] = dest[i - delta];
    }
    for (i = 0; i < delta; i++) {
        dest[pos++] = src[i];
    }

    return (EjsVar*) ap;
}


/*
 *  Joins the elements in the array into a single string.
 *  @param sep Element separator.
 *  @return Returns a string.
 *
 *  function join(sep: String = undefined): String
 */
static EjsVar *joinArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsString       *result, *sep;
    EjsVar          *vp;
    int             i;

    if (argc == 1) {
        sep = (EjsString*) argv[0];
    } else {
        sep = 0;
    }

    result = ejsCreateString(ejs, "");
    for (i = 0; i < ap->length; i++) {
        vp = ap->data[i];
        if (vp == 0 || ejsIsUndefined(vp) || ejsIsNull(vp)) {
            continue;
        }
        if (i > 0 && sep) {
            ejsStrcat(ejs, result, (EjsVar*) sep);
        }
        ejsStrcat(ejs, result, vp);
    }
    return (EjsVar*) result;
}


/*
 *  Search for an item using strict equality "===". This call searches from
 *  the end of the array for the specified element.
 *  @return Returns the items index into the array if found, otherwise -1.
 *
 *  function lastIndexOf(element: Object, fromIndex: Number = 0): Number
 */
static EjsVar *lastArrayIndexOf(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar          *element;
    int             i, start;

    mprAssert(argc == 1 || argc == 2);

    element = argv[0];
    start = ((argc == 2) ? (int) ((EjsNumber*) argv[1])->value : ap->length - 1);

    for (i = start; i >= 0; i--) {
        if (compare(ejs, ap->data[i], element)) {
            return (EjsVar*) ejsCreateNumber(ejs, i);
        }
    }
    return (EjsVar*) ejs->minusOneValue;
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic override function get length(): Number
 */

static EjsVar *getArrayLength(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


/*
 *  Set the length of an array.
 *
 *  intrinsic override function set length(value: Number): void
 */

static EjsVar *setArrayLength(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      **data, **dest;
    int         length;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    mprAssert(ejsIsArray(ap));

    length = (int) ((EjsNumber*) argv[0])->value;

    if (length > ap->length) {
        if (growArray(ejs, ap, length) < 0) {
            return 0;
        }
        data = ap->data;
        for (dest = &data[ap->length]; dest < &data[length]; dest++) {
            *dest = 0;
        }
    }

    ap->length = length;
    return 0;
}


/*
 *  Remove and return the last value in the array.
 *  @return Returns the last element in the array.
 *
 *  function pop(): Object
 */
static EjsVar *popArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    if (ap->length == 0) {
        return (EjsVar*) ejs->undefinedValue;
    }
    return ap->data[--ap->length];
}


/*
 *  Append items to the end of the array.
 *  @return Returns the new length of the array.
 *
 *  function push(...items): Number
 */
static EjsVar *pushArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      **src, **dest;
    int         i, oldLen;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    oldLen = ap->length;
    if (growArray(ejs, ap, ap->length + args->length) < 0) {
        return 0;
    }

    dest = ap->data;
    src = args->data;
    for (i = 0; i < args->length; i++) {
        dest[i + oldLen] = src[i];
    }
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


/*
 *  Reverse the order of the objects in the array. The elements are reversed in the original array.
 *  @return Returns a reference to the array.
 *
 *  function reverse(): Array
 */
static EjsVar *reverseArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar  *tmp, **data;
    int     i, j;

    if (ap->length <= 1) {
        return (EjsVar*) ap;
    }

    data = ap->data;
    i = (ap->length - 2) / 2;
    j = (ap->length + 1) / 2;

    for (; i >= 0; i--, j++) {
        tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
    }
    return (EjsVar*) ap;
}


/*
 *  Remove and return the first value in the array.
 *  @return Returns the first element in the array.
 *
 *  function shift(): Object
 */
static EjsVar *shiftArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      *result, **data;
    int         i;

    if (ap->length == 0) {
        return ejs->undefinedValue;
    }

    data = ap->data;
    result = data[0];

    for (i = 1; i < ap->length; i++) {
        data[i - 1] = data[i];
    }
    ap->length--;

    return result;
}


/*
 *  Create a new array by taking a slice from an array.
 *
 *  function slice(start: Number, end: Number, step: Number = 1): Array
 */
static EjsVar *sliceArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *result;
    EjsVar      **src, **dest;
    int         start, end, step, i, j, len;

    mprAssert(1 <= argc && argc <= 3);

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        end = ejsGetInt(argv[1]);
    } else {
        end = ap->length;
    }
    if (argc == 3) {
        step = ejsGetInt(argv[2]);
    } else {
        step = 1;
    }
    if (step == 0) {
        step = 1;
    }

    if (start < 0) {
        start += ap->length;
    }
    if (start < 0) {
        start = 0;
    } else if (start >= ap->length) {
        start = ap->length;
    }

    if (end < 0) {
        end += ap->length;
    }
    if (end < 0) {
        end = 0;
    } else if (end >= ap->length) {
        end = ap->length;
    }

    /*
     *  This may allocate too many elements if step is > 0, but length will still be correct.
     */
    result = ejsCreateArray(ejs, end - start);
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }
    src = ap->data;
    dest = result->data;

    len = 0;
    if (step > 0) {
        for (i = start, j = 0; i < end; i += step, j++) {
            dest[j] = src[i];
            len++;
        }

    } else {
        for (i = start, j = 0; i > end; i += step, j++) {
            dest[j] = src[i];
            len++;
        }
    }

    result->length = len;

    return (EjsVar*) result;
}


static int partition(Ejs *ejs, EjsVar **data, int p, int r)
{
    EjsVar          *tmp, *x;
    EjsString       *sx, *so;
    int             i, j;

    x = data[r];
    j = p - 1;

    for (i = p; i < r; i++) {

        sx = ejsToString(ejs, x);
        so = ejsToString(ejs, data[i]);
        if (sx == 0 || so == 0) {
            return 0;
        }
        if (strcmp(sx->value, so->value) > 0) {
            j = j + 1;
            tmp = data[j];
            data[j] = data[i];
            data[i] = tmp;
        }
    }

    data[r] = data[j + 1];
    data[j + 1] = x;

    return j + 1;
}


void quickSort(Ejs *ejs, EjsArray *ap, int p, int r)
{
    int     q;

    if (p < r) {
        q = partition(ejs, ap->data, p, r);
        quickSort(ejs, ap, p, q - 1);
        quickSort(ejs, ap, q + 1, r);
    }
}


/**
 *  Sort the array using the supplied compare function
 *  intrinsic native function sort(compare: Function = null, order: Number = 1): Array
 */
static EjsVar *sortArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    if (ap->length <= 1) {
        return (EjsVar*) ap;
    }
    quickSort(ejs, ap, 0, ap->length - 1);
    return (EjsVar*) ap;
}


/*
 *  Insert, remove or replace array elements. Return the removed elements.
 *
 *  function splice(start: Number, deleteCount: Number, ...values): Array
 *
 */
static EjsVar *spliceArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *result, *values;
    EjsVar      **data, **dest, **items;
    int         start, deleteCount, i, delta, endInsert, oldLen;

    mprAssert(1 <= argc && argc <= 3);

    start = ejsGetInt(argv[0]);
    deleteCount = ejsGetInt(argv[1]);
    values = (EjsArray*) argv[2];

    if (start < 0) {
        start += ap->length;
    }
    if (start >= ap->length) {
        start = ap->length - 1;
    }

    if (deleteCount < 0) {
        deleteCount = ap->length - start + 1;
    }

    result = ejsCreateArray(ejs, deleteCount);
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    data = ap->data;
    dest = result->data;
    items = values->data;

    /*
     *  Copy removed items to the result
     */
    for (i = 0; i < deleteCount; i++) {
        dest[i] = data[i + start];
    }

    oldLen = ap->length;
    delta = values->length - deleteCount;
    
    if (delta > 0) {
        /*
         *  Make room for items to insert
         */
        if (growArray(ejs, ap, ap->length + delta) < 0) {
            return 0;
        }
        endInsert = start + delta;
        for (i = ap->length - 1; i >= endInsert; i--) {
            data[i] = data[i - delta];
        }
        
    } else {
        ap->length += delta;
    }

    /*
     *  Copy in new values
     */
    for (i = 0; i < values->length; i++) {
        data[start + i] = items[i];
    }

    /*
     *  Remove holes
     */
    if (delta < 0) {
        for (i = start + values->length; i < oldLen; i++) {
            data[i] = data[i - delta];
        }
    }
    
    return (EjsVar*) result;
}



#if ES_Object_toLocaleString
/*
 *  Convert the array to a single localized string each member of the array
 *  has toString called on it and the resulting strings are concatenated.
 *  Currently just calls toString.
 *
 *  function toLocaleString(): String
 */
static EjsVar *toLocaleString(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return 0;
}
#endif


/*
 *  Convert the array to a single string each member of the array has toString called on it and the resulting strings 
 *  are concatenated.
 *
 *  override function toString(): String
 */
static EjsVar *arrayToString(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsString       *result;
    EjsVar          *vp;
    int             i, rc;

    result = ejsCreateString(ejs, "");
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    for (i = 0; i < ap->length; i++) {
        vp = ap->data[i];
        rc = 0;
        if (i > 0) {
            //  TODO OPT - this is slow
            rc = ejsStrcat(ejs, result, (EjsVar*) ejsCreateString(ejs, ","));
        }
        if (vp != 0 && vp != ejs->undefinedValue && vp != ejs->nullValue) {
            //  TODO OPT - this is slow
            rc = ejsStrcat(ejs, result, vp);
        }
        if (rc < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }
    return (EjsVar*) result;
}


/*
 *  Return an array with duplicate elements removed where duplicates are detected by using "==" (ie. content equality, 
 *  not strict equality).
 *
 *  function unique(): Array
 */
static EjsVar *uniqueArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      **data;
    int         i, j, k;

    data = ap->data;

    for (i = 0; i < ap->length; i++) {
        for (j = i + 1; j < ap->length; j++) {
            if (compare(ejs, data[i], data[j])) {
                for (k = j + 1; k < ap->length; k++) {
                    data[k - 1] = data[k];
                }
                ap->length--;
            }
        }
    }
    return (EjsVar*) ap;
}



static int growArray(Ejs *ejs, EjsArray *ap, int len)
{
    int         size, count, factor;

    mprAssert(ap);

    if (len <= 0) {
        return 0;
    }

    if (len <= ap->length) {
        return EJS_ERR;
    }

    size = mprGetBlockSize(ap->data) / sizeof(EjsVar*);

    /*
     *  Allocate or grow the data structures
     */
    if (len > size) {
        if (size > EJS_LOTSA_PROP) {
            /*
             *  Looks like a big object so grow by a bigger chunk
             */
            factor = max(size / 4, EJS_NUM_PROP);
            len = (len + factor) / factor * factor;
        }
        count = EJS_PROP_ROUNDUP(len);

        if (ap->data == 0) {
            mprAssert(ap->length == 0);
            mprAssert(count > 0);
            ap->data = (EjsVar**) mprAllocZeroed(ap, sizeof(EjsVar*) * count);
            if (ap->data == 0) {
                return EJS_ERR;
            }

        } else {
            mprAssert(size > 0);
            ap->data = (EjsVar**) mprRealloc(ap, ap->data, sizeof(EjsVar*) * count);
            if (ap->data == 0) {
                return EJS_ERR;
            }
            ejsZeroSlots(ejs, &ap->data[ap->length], (count - ap->length));
        }
    }
    ap->length = len;

    return 0;
}



EjsArray *ejsCreateArray(Ejs *ejs, int size)
{
    EjsArray    *ap;

    /*
     *  No need to invoke constructor
     */
    ap = (EjsArray*) ejsCreateObject(ejs, ejs->arrayType, 0);
    if (ap != 0) {
        if (size > 0 && growArray(ejs, ap, size) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }

    ejsSetDebugName(ap, "array instance");

    return ap;
}


void ejsCreateArrayType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Array"), ejs->objectType, sizeof(EjsArray),
        ES_Array, ES_Array_NUM_CLASS_PROP, ES_Array_NUM_INSTANCE_PROP,
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_HAS_CONSTRUCTOR | 
        EJS_ATTR_OBJECT_HELPERS |EJS_ATTR_OPER_OVERLOAD);
    ejs->arrayType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castArray;
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneArray;
    type->helpers->getProperty = (EjsGetPropertyHelper) getArrayProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getArrayPropertyCount;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getArrayPropertyByName;
    type->helpers->deleteProperty = (EjsDeletePropertyHelper) deleteArrayProperty;
    type->helpers->deletePropertyByName = (EjsDeletePropertyByNameHelper) deleteArrayPropertyByName;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeArrayOperator;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupArrayProperty;
    type->helpers->markVar = (EjsMarkVarHelper) markArrayVar;
    type->helpers->setProperty = (EjsSetPropertyHelper) setArrayProperty;
    type->helpers->setPropertyByName = (EjsSetPropertyByNameHelper) setArrayPropertyByName;
    type->numericIndicies = 1;
}


void ejsConfigureArrayType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->arrayType;

    /*
     *  We override some object methods
     */
    ejsBindMethod(ejs, type, ES_Object_get, getArrayIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getArrayValues);
    ejsBindMethod(ejs, type, ES_Object_clone, (EjsNativeFunction) cloneArrayMethod);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) arrayToString);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_Object_length, (EjsNativeFunction) getArrayLength, ES_Array_set_length, 
        (EjsNativeFunction) setArrayLength);
#else
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) getArrayLength);
    ejsBindMethod(ejs, type, ES_Array_set_length, (EjsNativeFunction) setArrayLength);
#endif

    /*
     *  Methods and Operators, including constructor.
     */
    ejsBindMethod(ejs, type, ES_Array_Array, (EjsNativeFunction) arrayConstructor);
    ejsBindMethod(ejs, type, ES_Array_append, (EjsNativeFunction) appendArray);
    ejsBindMethod(ejs, type, ES_Array_clear, (EjsNativeFunction) clearArray);
    ejsBindMethod(ejs, type, ES_Array_compact, (EjsNativeFunction) compactArray);
    ejsBindMethod(ejs, type, ES_Array_concat, (EjsNativeFunction) concatArray);

    ejsBindMethod(ejs, type, ES_Array_indexOf, (EjsNativeFunction) indexOfArray);
    ejsBindMethod(ejs, type, ES_Array_insert, (EjsNativeFunction) insertArray);
    ejsBindMethod(ejs, type, ES_Array_join, (EjsNativeFunction) joinArray);
    ejsBindMethod(ejs, type, ES_Array_lastIndexOf, (EjsNativeFunction) lastArrayIndexOf);
    ejsBindMethod(ejs, type, ES_Array_pop, (EjsNativeFunction) popArray);
    ejsBindMethod(ejs, type, ES_Array_push, (EjsNativeFunction) pushArray);
    ejsBindMethod(ejs, type, ES_Array_reverse, (EjsNativeFunction) reverseArray);
    ejsBindMethod(ejs, type, ES_Array_shift, (EjsNativeFunction) shiftArray);
    ejsBindMethod(ejs, type, ES_Array_slice, (EjsNativeFunction) sliceArray);
    ejsBindMethod(ejs, type, ES_Array_sort, (EjsNativeFunction) sortArray);
    ejsBindMethod(ejs, type, ES_Array_splice, (EjsNativeFunction) spliceArray);
    ejsBindMethod(ejs, type, ES_Array_unique, (EjsNativeFunction) uniqueArray);

#if FUTURE
    ejsBindMethod(ejs, type, ES_Array_toLocaleString, toLocaleString);
    ejsBindMethod(ejs, type, ES_Array_toJSONString, toJSONString);
    ejsBindMethod(ejs, type, ES_Array_LBRACKET, operLBRACKET);
    ejsBindMethod(ejs, type, ES_Array_AND, operAND);
    ejsBindMethod(ejs, type, ES_Array_EQ, operEQ);
    ejsBindMethod(ejs, type, ES_Array_GT, operGT);
    ejsBindMethod(ejs, type, ES_Array_LT, operLT);
    ejsBindMethod(ejs, type, ES_Array_LSH, operLSH);
    ejsBindMethod(ejs, type, ES_Array_MINUS, operMINUS);
    ejsBindMethod(ejs, type, ES_Array_OR, operOR);
    ejsBindMethod(ejs, type, ES_Array_AND, operAND);
#endif
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsArray.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsBlock.c"
 */
/************************************************************************/

/**
 *  ejsBlock.c - Lexical block
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int growTraits(EjsBlock *block, int numTraits);
static int insertGrowTraits(EjsBlock *block, int count, int offset);


EjsBlock *ejsCreateBlock(Ejs *ejs, cchar *name, int size)
{
    EjsBlock        *block;

    block = (EjsBlock*) ejsCreateObject(ejs, ejs->blockType, size);
    if (block == 0) {
        return 0;
    }

    ejsInitList(&block->namespaces);

    //  TODO - really should remove this name somehow
    block->name = name;
    ejsSetFmtDebugName(block, "block %s", name);

    return block;
}


/*
 *  Define a new property and set its name, type, attributes and property value.
 */
static int defineBlockProperty(Ejs *ejs, EjsBlock *block, int slotNum, EjsName *qname, EjsType *propType, int attributes, 
    EjsVar *val)
{
    EjsFunction     *fun;
    EjsType         *type;

    mprAssert(ejs);
    mprAssert(slotNum >= -1);
    mprAssert(ejsIsObject(block));
    mprAssert(qname);

    if (val == 0) {
        val = ejs->nullValue;
    }

    if (slotNum < 0) {
        slotNum = ejsGetPropertyCount(ejs, (EjsVar*) block);
    }

    if (ejsSetProperty(ejs, (EjsVar*) block, slotNum, val) < 0) {
        return EJS_ERR;
    }
    if (ejsSetPropertyName(ejs, (EjsVar*) block, slotNum, qname) < 0) {
        return EJS_ERR;
    }
    if (ejsSetPropertyTrait(ejs, (EjsVar*) block, slotNum, propType, attributes) < 0) {
        return EJS_ERR;
    }

    if (ejsIsFunction(val)) {
        fun = ((EjsFunction*) val);
        if (attributes & EJS_ATTR_CONSTRUCTOR) {
            fun->constructor = 1;
        }
        ejsSetFunctionLocation(fun, (EjsVar*) block, slotNum);
        if (fun->getter || fun->setter) {
             block->obj.var.hasGetterSetter = 1;
        }
        if (!ejsIsNativeFunction(fun)) {
            block->hasScriptFunctions = 1;
        }
    }

    if (ejsIsType(block) && !isalpha((int) qname->name[0])) {
        type = (EjsType*) block;
        type->operatorOverload = 1;
    }

    return slotNum;
}


/*
 *  Get the property Trait
 */
static EjsTrait *getBlockPropertyTrait(Ejs *ejs, EjsBlock *block, int slotNum)
{
    return ejsGetTrait(block, slotNum);
}


void ejsMarkBlock(Ejs *ejs, EjsVar *parent, EjsBlock *block)
{
    EjsVar          *item;
    EjsBlock        *b;
    int             next;

    ejsMarkObject(ejs, parent, (EjsObject*) block);

    if (block->namespaces.length > 0) {
        for (next = 0; ((item = (EjsVar*) ejsGetNextItem(&block->namespaces, &next)) != 0); ) {
            ejsMarkVar(ejs, (EjsVar*) block, item);
        }
    }

    for (b = block->scopeChain; b; b = b->scopeChain) {
        ejsMarkVar(ejs, (EjsVar*) block, (EjsVar*) b);
    }
}


/*
 *  Set the property Trait
 */
static int setBlockPropertyTrait(Ejs *ejs, EjsBlock *block, int slotNum, EjsType *type, int attributes)
{
    return ejsSetTrait(block, slotNum, type, attributes);
}



/*
 *  Grow the block traits, slots and names. This will update numTraits and numProp.
 */
int ejsGrowBlock(Ejs *ejs, EjsBlock *block, int size)
{
    if (size == 0) {
        return 0;
    }
    if (ejsGrowObject(ejs, (EjsObject*) block, size) < 0) {
        return EJS_ERR;
    }
    if (growTraits(block, size) < 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Grow the block traits, slots and names by inserting before all existing properties. This will update numTraits and numProp.
 */
int ejsInsertGrowBlock(Ejs *ejs, EjsBlock *block, int count, int offset)
{
    EjsFunction     *fun;
    int             i;

    if (count <= 0) {
        return 0;
    }
    if (ejsInsertGrowObject(ejs, (EjsObject*) block, count, offset) < 0) {
        return EJS_ERR;
    }
    if (insertGrowTraits(block, count, offset) < 0) {
        return EJS_ERR;
    }

    /*
     *  Fixup the slot numbers of all the methods.
     */
    for (i = offset + count; i < block->numTraits; i++) {
        fun = (EjsFunction*) block->obj.slots[i];
        if (fun == 0 || !ejsIsFunction(fun)) {
            continue;
        }
        fun->slotNum += count;
        if (fun->nextSlot >= 0) {
            fun->nextSlot += count;
        }
        mprAssert(fun->slotNum == i);
        mprAssert(fun->nextSlot < block->numTraits);
    }

    return 0;
}


/*
 *  Allocate space for traits. Caller will initialize the actual traits.
 */
static int growTraits(EjsBlock *block, int numTraits)
{
    int         count;

    mprAssert(block);
    mprAssert(numTraits >= 0);

    if (numTraits == 0) {
        return 0;
    }

    if (numTraits > block->sizeTraits) {
        count = EJS_PROP_ROUNDUP(numTraits);
        block->traits = (EjsTrait*) mprRealloc(block, block->traits, sizeof(EjsTrait) * count);
        if (block->traits == 0) {
            return EJS_ERR;
        }
        memset(&block->traits[block->sizeTraits], 0, (count - block->sizeTraits) * sizeof(EjsTrait));
        block->sizeTraits = count;
    }

    if (numTraits > block->numTraits) {
        block->numTraits = numTraits;
    }

    mprAssert(block->numTraits <= block->sizeTraits);
    mprAssert(block->sizeTraits <= block->obj.capacity);
    mprAssert(block->sizeTraits <= block->obj.names->sizeEntries);

    return 0;
}


static int insertGrowTraits(EjsBlock *block, int count, int offset)
{
    int         mark, i;

    mprAssert(block);
    mprAssert(count >= 0);

    if (count == 0) {
        return 0;
    }

    /*
     *  This call will change both numTraits and sizeTraits
     */
    growTraits(block, block->numTraits + count);

    mark = offset + count ;
    for (i = block->numTraits - 1; i >= mark; i--) {
        block->traits[i] = block->traits[i - mark];
    }
    for (; i >= offset; i--) {
        block->traits[i].attributes = 0;
        block->traits[i].type = 0;
    }

    mprAssert(block->numTraits <= block->sizeTraits);

    return 0;
}


/*
 *  Add a new trait to the trait array.
 *  TODO - should have ejs as first arg.
 */
int ejsSetTrait(EjsBlock *block, int slotNum, EjsType *type, int attributes)
{
    mprAssert(block);
    mprAssert(slotNum >= 0);

    if (slotNum < 0 || slotNum >= block->obj.capacity) {
        return EJS_ERR;
    }

    mprAssert(block->numTraits <= block->sizeTraits);

    if (block->sizeTraits <= slotNum) {
        growTraits(block, slotNum + 1);
    } else if (block->numTraits <= slotNum) {
        block->numTraits = slotNum + 1;
    }
    
    block->traits[slotNum].type = type;
    block->traits[slotNum].attributes = attributes;
    
    mprAssert(block->numTraits <= block->sizeTraits);
    mprAssert(slotNum < block->sizeTraits);

    return slotNum;
}


void ejsSetTraitType(EjsTrait *trait, EjsType *type)
{
    mprAssert(trait);
    mprAssert(type == 0 || ejsIsType(type));

    trait->type = type;
}


void ejsSetTraitAttributes(EjsTrait *trait, int attributes)
{
    mprAssert(trait);

    trait->attributes = attributes;
}


/*
 *  Remove the designated slot. If compact is true, then copy slots down.
 */
static int removeTrait(EjsBlock *block, int slotNum, int compact)
{
    int         i;

    mprAssert(block);
    mprAssert(block->numTraits <= block->sizeTraits);
    
    if (slotNum < 0 || slotNum >= block->numTraits) {
        return EJS_ERR;
    }

    if (compact) {
        /*
         *  Copy traits down starting at
         */
        for (i = slotNum + 1; i < block->numTraits; i++) {
            block->traits[i - 1] = block->traits[i];
        }
        block->numTraits--;
        i--;

    } else {
        i = slotNum;
    }

    mprAssert(i >= 0);
    mprAssert(i < block->sizeTraits);

    block->traits[i].attributes = 0;
    block->traits[i].type = 0;

    if ((i - 1) == block->numTraits) {
        block->numTraits--;
    }

    mprAssert(slotNum < block->sizeTraits);
    mprAssert(block->numTraits <= block->sizeTraits);

    return 0;
}


/*
 *  Copy inherited type slots and traits. Don't copy overridden properties and clear property names for static properites
 */
int ejsInheritTraits(Ejs *ejs, EjsBlock *block, EjsBlock *baseBlock, int count, int offset, bool implementing)
{
    EjsNames        *names;
    EjsTrait        *trait;
    EjsFunction     *fun, *existingFun;
    EjsObject       *obj;
    int             i, start;

    mprAssert(block);
    
    if (baseBlock == 0 || count <= 0) {
        return 0;
    }
    block->numInherited += count;
    
    mprAssert(block->numInherited <= block->numTraits);
    mprAssert(block->numTraits <= block->sizeTraits);

    obj = &block->obj;
    names = obj->names;

    start = baseBlock->numTraits - count;
    for (i = start; i < baseBlock->numTraits; i++, offset++) {
        trait = &block->traits[i];

        if (obj->var.isInstanceBlock) {
            /*
             *  Instance properties
             */
            obj->slots[offset] = baseBlock->obj.slots[i];
            block->traits[offset] = baseBlock->traits[i];
            names->entries[offset] = baseBlock->obj.names->entries[i];

        } else {

            existingFun = (EjsFunction*) block->obj.slots[offset];
            if (existingFun && ejsIsFunction(existingFun) && existingFun->override) {
                continue;
            }

            /*
             *  Copy implemented properties (including static and instance functions) and inherited instance functions.
             */
            fun = (EjsFunction*) baseBlock->obj.slots[i];
            if (implementing || (fun && ejsIsFunction(fun) && !fun->staticMethod)) {
                obj->slots[offset] = (EjsVar*) fun;
                block->traits[offset] = baseBlock->traits[i];
                names->entries[offset] = baseBlock->obj.names->entries[i];
            }
        }
    }

    if (block->numTraits < block->numInherited) {
        block->numTraits = block->numInherited;
        mprAssert(block->numTraits <= block->sizeTraits);
    }

    ejsRebuildHash(ejs, obj);
    return 0;
}


/*
 *  Get a trait by slot number. (Note: use ejsGetPropertyTrait for access to a property's trait)
 */
EjsTrait *ejsGetTrait(EjsBlock *block, int slotNum)
{
    mprAssert(block);
    mprAssert(slotNum >= 0);

    if (slotNum < 0 || slotNum >= block->numTraits) {
        return 0;
    }
    mprAssert(slotNum < block->numTraits);
    return &block->traits[slotNum];
}


int ejsGetTraitAttributes(EjsBlock *block, int slotNum)
{
    mprAssert(block);
    mprAssert(slotNum >= 0);

    if (slotNum < 0 || slotNum >= block->numTraits) {
        mprAssert(0);
        return 0;
    }
    mprAssert(slotNum < block->numTraits);
    return block->traits[slotNum].attributes;
}


EjsType *ejsGetTraitType(EjsBlock *block, int slotNum)
{
    mprAssert(block);
    mprAssert(slotNum >= 0);

    if (slotNum < 0 || slotNum >= block->numTraits) {
        mprAssert(0);
        return 0;
    }
    mprAssert(slotNum < block->numTraits);
    return block->traits[slotNum].type;
}


int ejsGetNumTraits(EjsBlock *block)
{
    mprAssert(block);

    return block->numTraits;
}


int ejsGetNumInheritedTraits(EjsBlock *block)
{
    mprAssert(block);
    return block->numInherited;
}


/*
 *  Remove a property from a block. This erases the property and its traits.
 *  TODO - OPT only used by the compiler.
 */
int ejsRemoveProperty(Ejs *ejs, EjsBlock *block, int slotNum)
{
    EjsFunction     *fun;
    EjsVar          *vp;
    int             i;

    mprAssert(ejs);
    mprAssert(block);
    
    /*
     *  Copy type slots and traits down to remove the slot
     */
    removeTrait(block, slotNum, 1);
    ejsRemoveSlot(ejs, (EjsObject*) block, slotNum, 1);

    mprAssert(block->numTraits <= block->obj.numProp);

    /*
     *  Fixup the slot numbers of all the methods
     */
    for (i = slotNum; i < block->obj.numProp; i++) {
        vp = block->obj.slots[i];
        if (vp == 0) {
            continue;
        }
        if (ejsIsFunction(vp)) {
            fun = (EjsFunction*) vp;
            fun->slotNum--;
            mprAssert(fun->slotNum == i);
            if (fun->nextSlot >= 0) {
                fun->nextSlot--;
                mprAssert(fun->slotNum < block->obj.numProp);
            }
        }
    }

    return 0;
}


EjsBlock *ejsCopyBlock(Ejs *ejs, EjsBlock *src, bool deep)
{
    EjsBlock    *dest;

    dest = (EjsBlock*) ejsCopyObject(ejs, (EjsObject*) src, deep);

    dest->numTraits = src->numTraits;
    dest->sizeTraits = src->sizeTraits;
    dest->traits = src->traits;
    dest->numInherited = src->numInherited;
    dest->scopeChain = src->scopeChain;
    dest->name = src->name;
    
    mprAssert(dest->numTraits <= dest->sizeTraits);

    if (ejsCopyList(dest, &dest->namespaces, &src->namespaces) < 0) {
        return 0;
    }

    return dest;
}



void ejsResetBlockNamespaces(Ejs *ejs, EjsBlock *block)
{
    ejsClearList(&block->namespaces);
}


int ejsGetNamespaceCount(EjsBlock *block)
{
    mprAssert(block);

    return ejsGetListCount(&block->namespaces);
}


void ejsPopBlockNamespaces(EjsBlock *block, int count)
{
    mprAssert(block);
    mprAssert(block->namespaces.length >= count);

    block->namespaces.length = count;
}


int ejsAddNamespaceToBlock(Ejs *ejs, EjsBlock *block, EjsNamespace *namespace)
{
    mprAssert(block);

    if (namespace == 0) {
        ejsThrowTypeError(ejs, "Not a namespace");
        return EJS_ERR;
    }

    ejsAddItemToSharedList(block, &block->namespaces, namespace);
    ejsSetReference(ejs, (EjsVar*) block, (EjsVar*) namespace);
    return 0;
}


/*
 *  Inherit namespaces from base types. Only inherit protected.
 */
void ejsInheritBaseClassNamespaces(Ejs *ejs, EjsType *type, EjsType *baseType)
{
    EjsNamespace    *nsp;
    EjsBlock        *block;
    EjsList         *baseNamespaces, oldNamespaces;
    int             next;

    block = &type->block;
    oldNamespaces = block->namespaces;
    ejsInitList(&block->namespaces);
    baseNamespaces = &baseType->block.namespaces;

    if (baseNamespaces) {
        for (next = 0; ((nsp = (EjsNamespace*) ejsGetNextItem(baseNamespaces, &next)) != 0); ) {
            if (strstr(nsp->name, ",protected")) {
                ejsAddItem(block, &block->namespaces, nsp);
            }
        }
    }

    if (oldNamespaces.length > 0) {
        for (next = 0; ((nsp = (EjsNamespace*) ejsGetNextItem(&oldNamespaces, &next)) != 0); ) {
            ejsAddItem(block, &block->namespaces, nsp);
        }
    }
}



void ejsCreateBlockType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Block"), ejs->objectType, 
        sizeof(EjsType), ES_Block, ES_Block_NUM_CLASS_PROP, ES_Block_NUM_INSTANCE_PROP, 
        EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_BLOCK_HELPERS);
    type->skipScope = 1;
    ejs->blockType = type;
}


void ejsConfigureBlockType(Ejs *ejs)
{
}


void ejsInitializeBlockHelpers(EjsTypeHelpers *helpers)
{
    helpers->cloneVar               = (EjsCloneVarHelper) ejsCopyBlock;
    helpers->defineProperty         = (EjsDefinePropertyHelper) defineBlockProperty;
    helpers->getPropertyTrait       = (EjsGetPropertyTraitHelper) getBlockPropertyTrait;
    helpers->markVar                = (EjsMarkVarHelper) ejsMarkBlock;
    helpers->setPropertyTrait       = (EjsSetPropertyTraitHelper) setBlockPropertyTrait;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsBlock.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsBoolean.c"
 */
/************************************************************************/

/**
 *  ejsBoolean.c - Boolean native class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Cast the operand to a primitive type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castBooleanVar(Ejs *ejs, EjsBoolean *vp, EjsType *type)
{
    mprAssert(ejsIsBoolean(vp));

    switch (type->id) {

    case ES_Number:
        return (EjsVar*) ((vp->value) ? ejs->oneValue: ejs->zeroValue);

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, (vp->value) ? "true" : "false");

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


/*
 *  Coerce operands for invokeOperator
 */
static EjsVar *coerceBooleanOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {

    case EJS_OP_ADD:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->nanValue;
        } else if (ejsIsNull(rhs) || ejsIsNumber(rhs) || ejsIsDate(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        } else {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
        if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) (((EjsBoolean*) lhs)->value ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsBoolean*) lhs)->value ? ejs->falseValue : ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
}


/*
 *  Run an operator on the operands
 */
static EjsVar *invokeBooleanOperator(Ejs *ejs, EjsBoolean *lhs, int opcode, EjsBoolean *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceBooleanOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ((lhs->value == rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ((lhs->value != rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ((lhs->value < rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ((lhs->value <= rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ((lhs->value > rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ((lhs->value >= rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) ((lhs->value) ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    /*
     *  Unary operators
     */
    case EJS_OP_NEG:
        return (EjsVar*) ejsCreateNumber(ejs, - lhs->value);

    case EJS_OP_LOGICAL_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, !lhs->value);

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, ~lhs->value);

    /*
     *  Binary operations
     */
    case EJS_OP_ADD:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value + rhs->value);

    case EJS_OP_AND:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value & rhs->value);

    case EJS_OP_DIV:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value / rhs->value);

    case EJS_OP_MUL:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value * rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value | rhs->value);

    case EJS_OP_REM:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value % rhs->value);

    case EJS_OP_SUB:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value - rhs->value);

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value >> rhs->value);

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value ^ rhs->value);

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
}


/*
 *  Boolean constructor.
 *
 *      function Boolean()
 *      function Boolean(value: Boolean)
 *
 *  If the value is omitted or 0, -1, NaN, false, null, undefined or the empty string, then set the boolean value to
 *  to false.
 */

static EjsVar *booleanConstructor(Ejs *ejs, EjsBoolean *bp, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      *vp;

    mprAssert(argc == 0 || (argc == 1 && ejsIsArray(argv[0])));

    if (argc == 1) {
        args = (EjsArray*) argv[0];

        vp = ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (vp) {
            /* Change the bp value */
            bp = ejsToBoolean(ejs, vp);
        }
    }
    return (EjsVar*) bp;
}



EjsBoolean *ejsCreateBoolean(Ejs *ejs, int value)
{
    return (value) ? ejs->trueValue : ejs->falseValue;
}


static void defineBooleanConstants(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->booleanType;

    if (ejs->flags & EJS_FLAG_EMPTY) {
        return;
    }

    ejsSetProperty(ejs, ejs->global, ES_boolean, (EjsVar*) type);
    ejsSetProperty(ejs, ejs->global, ES_true, (EjsVar*) ejs->trueValue);
    ejsSetProperty(ejs, ejs->global, ES_false, (EjsVar*) ejs->falseValue);
}


void ejsCreateBooleanType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Boolean"), ejs->objectType, sizeof(EjsBoolean),
        ES_Boolean, ES_Boolean_NUM_CLASS_PROP, ES_Boolean_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->booleanType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castBooleanVar;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeBooleanOperator;

    /*
     *  Pre-create the only two valid instances for boolean
     */
    ejs->trueValue = (EjsBoolean*) ejsCreateVar(ejs, type, 0);
    ejs->trueValue->value = 1;

    ejs->falseValue = (EjsBoolean*) ejsCreateVar(ejs, type, 0);
    ejs->falseValue->value = 0;

    ejsSetDebugName(ejs->falseValue, "false");
    ejsSetDebugName(ejs->trueValue, "true");

    defineBooleanConstants(ejs);
}


void ejsConfigureBooleanType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->booleanType;

    defineBooleanConstants(ejs);
    ejsBindMethod(ejs, type, ES_Boolean_Boolean, (EjsNativeFunction) booleanConstructor);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsBoolean.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsByteArray.c"
 */
/************************************************************************/

/*
 *  ejsByteArray.c - Ejscript ByteArray class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int  flushByteArray(Ejs *ejs, EjsByteArray *ap);
static int  getInput(Ejs *ejs, EjsByteArray *ap, int required);
static int  growByteArray(Ejs *ejs, EjsByteArray *ap, int len);
static int  lookupByteArrayProperty(Ejs *ejs, EjsByteArray *ap, EjsName *qname);
 static bool makeRoom(Ejs *ejs, EjsByteArray *ap, int require);
static EjsVar *byteArrayToString(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv);

static MPR_INLINE int swap16(EjsByteArray *ap, int a);
static MPR_INLINE int swap32(EjsByteArray *ap, int a);
static MPR_INLINE int64 swap64(EjsByteArray *ap, int64 a);
static MPR_INLINE double swapDouble(EjsByteArray *ap, double a);
static void putByte(EjsByteArray *ap, int value);
static void putInteger(EjsByteArray *ap, int value);
static void putLong(EjsByteArray *ap, int64 value);
static void putShort(EjsByteArray *ap, int value);
static void putString(EjsByteArray *ap, cchar *value, int len);
static void putNumber(EjsByteArray *ap, MprNumber value);

#if BLD_FEATURE_FLOATING_POINT
static void putDouble(EjsByteArray *ap, double value);
#endif

#define availableBytes(ap)  (((EjsByteArray*) ap)->writePosition - ((EjsByteArray*) ap)->readPosition)
#define room(ap) (ap->length - ap->writePosition)
#define adjustReadPosition(ap, amt) \
    if (1) { \
        ap->readPosition += amt; \
        if (ap->readPosition == ap->writePosition) {    \
            ap->readPosition = ap->writePosition = 0; \
        } \
    } else

/*
 *  Cast the object operand to a primitive type
 */

static EjsVar *castByteArrayVar(Ejs *ejs, EjsByteArray *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejs->zeroValue;

    case ES_String:
        return byteArrayToString(ejs, vp, 0, 0);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


static EjsByteArray *cloneByteArrayVar(Ejs *ejs, EjsByteArray *ap, bool deep)
{
    EjsByteArray    *newArray;
    int             i;

    newArray = ejsCreateByteArray(ejs, ap->length);
    if (newArray == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    for (i = 0; i < ap->length; i++) {
        newArray->value[i] = ap->value[i];
    }

    return newArray;
}


/*
 *  Delete a property and update the length
 */
static int deleteByteArrayProperty(struct Ejs *ejs, EjsByteArray *ap, int slot)
{
    if (slot >= ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad subscript");
        return EJS_ERR;
    }
    if ((slot + 1) == ap->length) {
        ap->length--;
        if (ap->readPosition >= ap->length) {
            ap->readPosition = ap->length - 1;
        }
        if (ap->writePosition >= ap->length) {
            ap->writePosition = ap->length - 1;
        }
    }

    if (ejsSetProperty(ejs, (EjsVar*) ap, slot, (EjsVar*) ejs->undefinedValue) < 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Return the number of elements in the array
 */
static int getByteArrayPropertyCount(Ejs *ejs, EjsByteArray *ap)
{
    return ap->length;
}


/*
 *  Get an array element. Slot numbers correspond to indicies.
 */
static EjsVar *getByteArrayProperty(Ejs *ejs, EjsByteArray *ap, int slotNum)
{
    if (slotNum < 0 || slotNum >= ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad array subscript");
        return 0;
    }
    return (EjsVar*) ejsCreateNumber(ejs, ap->value[slotNum]);
}


/*
 *  Lookup an array index.
 */
static int lookupByteArrayProperty(struct Ejs *ejs, EjsByteArray *ap, EjsName *qname)
{
    int     index;

    if (qname == 0 || ! isdigit((int) qname->name[0])) {
        return EJS_ERR;
    }
    index = atoi(qname->name);
    if (index < ap->length) {
        return index;
    }
    return EJS_ERR;
}


/*
 *  Cast operands as required for invokeOperator
 */
static EjsVar *coerceByteArrayOperands(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, byteArrayToString(ejs, (EjsByteArray*) lhs, 0, 0), opcode, rhs);

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
        if (ejsIsNull(rhs) || ejsIsUndefined(rhs)) {
            return (EjsVar*) ((opcode == EJS_OP_COMPARE_EQ) ? ejs->falseValue: ejs->trueValue);
        } else if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }

    return 0;
}


static EjsVar *invokeByteArrayOperator(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceByteArrayOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, (lhs == rhs));

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs == rhs));

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->oneValue;

    /*
     *  Binary operators
     */
    case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_REM:
    case EJS_OP_SHR: case EJS_OP_USHR: case EJS_OP_XOR:
        return (EjsVar*) ejs->zeroValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


static void markByteArrayVar(Ejs *ejs, EjsVar *parent, EjsByteArray *ap)
{
    mprAssert(ejsIsByteArray(ap));

    if (ap->input) {
        ejsMarkVar(ejs, (EjsVar*) ap, (EjsVar*) ap->input);
    }
}


/*
 *  Create or update an array elements. If slotNum is < 0, then create the next free array slot. If slotNum is greater
 *  than the array length, grow the array.
 */
static int setByteArrayProperty(struct Ejs *ejs, EjsByteArray *ap, int slotNum,  EjsVar *value)
{
    if (slotNum >= ap->length) {
        if (growByteArray(ejs, ap, slotNum + 1) < 0) {
            return EJS_ERR;
        }
    }
    if (ejsIsNumber(value)) {
        ap->value[slotNum] = ejsGetInt(value);
    } else {
        ap->value[slotNum] = ejsGetInt(ejsToNumber(ejs, value));
    }

    if (slotNum >= ap->length) {
        ap->length = slotNum + 1;
    }
    return slotNum;
}


/*
 *  ByteArray constructor.
 *
 *  function ByteArray(size: Number = -1, growable: Boolean = true): ByteArray
 */
static EjsVar *byteArrayConstructor(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    bool    growable;
    int     size;

    mprAssert(0 <= argc && argc <= 2);

    size = (argc >= 1) ? ejsGetInt(argv[0]) : MPR_BUFSIZE;
    if (size <= 0) {
        size = 1;
    }
    growable = (argc == 2) ? ejsGetBoolean(argv[1]): 1;

    if (growByteArray(ejs, ap, size) < 0) {
        return 0;
    }
    mprAssert(ap->value);
    ap->growable = growable;
    ap->growInc = MPR_BUFSIZE;
    ap->length = size;
    ap->endian = mprGetEndian(ejs);

    return (EjsVar*) ap;
}


/**
 *  Get the number of bytes that are currently available on this stream for reading.
 *
 *  function get available(): Number
 */
static EjsVar *availableProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->writePosition - ap->readPosition);
}


/*
 *  Copy data into the array. Data is written at the $destOffset.
 *
 *  function copyIn(destOffset: Number, src: ByteArray, srcOffset: Number = 0, count: Number = -1): Void
 */
static EjsVar *copyIn(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsByteArray    *src;
    int             i, destOffset, srcOffset, count;

    destOffset = ejsGetInt(argv[0]);
    src = (EjsByteArray*) argv[1];
    srcOffset = (argc > 2) ? ejsGetInt(argv[2]) : 0;
    count = (argc > 3) ? ejsGetInt(argv[3]) : MAXINT;

    if (srcOffset >= src->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad source data offset");
        return 0;
    }
    count = min(src->length - srcOffset, count);

    makeRoom(ejs, ap, destOffset + count);
    if ((destOffset + count) > src->length) {
        ejsThrowOutOfBoundsError(ejs, "Insufficient room for data");
        return 0;
    }

    for (i = 0; i < count; i++) {
        ap->value[destOffset++] = src->value[srcOffset++];
    }
    return 0;
}


/*
 *  Copy data from the array. Data is copied from the $srcOffset.
 *
 *  function copyOut(srcOffset: Number, dest: ByteArray, destOffset: Number = 0, count: Number = -1): Number
 */
static EjsVar *copyOut(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsByteArray    *dest;
    int             i, srcOffset, destOffset, count;

    srcOffset = ejsGetInt(argv[0]);
    dest = (EjsByteArray*) argv[1];
    destOffset = (argc > 2) ? ejsGetInt(argv[2]) : 0;
    count = (argc > 3) ? ejsGetInt(argv[3]) : MAXINT;

    if (srcOffset >= ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad source data offset");
        return 0;
    }
    count = min(ap->length - srcOffset, count);

    makeRoom(ejs, dest, destOffset + count);
    if ((destOffset + count) > dest->length) {
        ejsThrowOutOfBoundsError(ejs, "Insufficient room for data");
        return 0;
    }

    for (i = 0; i < count; i++) {
        dest->value[destOffset++] = ap->value[srcOffset++];
    }
    return 0;
}


/*
 *  Determine if the system is using little endian byte ordering
 *
 *  function get endian(): Number
 */
static EjsVar *endian(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->endian);
}


/*
 *  Set the system encoding to little or big endian.
 *
 *  function set endian(value: Number): Void
 */
static EjsVar *setEndian(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     endian;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    endian = ejsGetInt(argv[0]);
    if (endian != 0 && endian != 1) {
        ejsThrowArgError(ejs, "Bad endian value");
        return 0;
    }

    ap->endian = endian;
    ap->swap = (ap->endian != mprGetEndian(ejs));

    return 0;
}


/*
 *  Function to iterate and return the next element index.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextByteArrayKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsByteArray    *ap;

    ap = (EjsByteArray*) ip->target;
    if (!ejsIsByteArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < ap->readPosition) {
        ip->index = ap->readPosition;
    }
    if (ip->index < ap->writePosition) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getByteArrayIterator(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextByteArrayKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextByteArrayValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsByteArray    *ap;

    ap = (EjsByteArray*) ip->target;
    if (!ejsIsByteArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < ap->readPosition) {
        ip->index = ap->readPosition;
    }
    if (ip->index < ap->writePosition) {
        return (EjsVar*) ejsCreateNumber(ejs, ap->value[ip->index++]);
    }

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getByteArrayValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextByteArrayValue, 0, NULL);
}


/**
 *  Callback function to called when read data is required. Callback signature:
 *      function callback(buffer: ByteArray, offset: Number, count: Number): Number
 *
 *  function set input(value: Function): Void
 */
static EjsVar *inputByteArrayData(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsFunction(argv[0]));

    ap->input = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  Flush the data in the byte array and reset the read and write position pointers
 *
 *  function flush(): Void
 */
static EjsVar *flushProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    flushByteArray(ejs, ap);
    ap->writePosition = ap->readPosition = 0;
    return 0;
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic override function get length(): Number
 */
static EjsVar *getByteArrayLength(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


#if UNUSED && TODO
/*
 *  Set the length of an array.
 *
 *  intrinsic override function set length(value: Number): void
 */
static EjsVar *setByteArrayLength(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    mprAssert(ejsIsByteArray(ap));

    ap->length = ejsGetInt(argv[0]);
    if (ap->readPosition >= ap->length) {
        ap->readPosition = ap->length - 1;
    }
    if (ap->writePosition >= ap->length) {
        ap->writePosition = ap->length - 1;
    }

    return 0;
}
#endif


/**
 *  Function to call to flush data. Callback signature:
 *      function callback(...data): Number
 *
 *  function set output(value: Function): Void
 */
static EjsVar *output(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsFunction(argv[0]));

    ap->output = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  Read data from the array into another byte array. Data is read from the current read $position pointer.
 *  Data is written to the write position if offset is -1. Othwise at the given offset. If offset is < 0, the write position is
 *  updated.
 *
 *  function read(buffer: ByteArray, offset: Number = -1, count: Number = -1): Number
 */
static EjsVar *byteArrayReadProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsByteArray    *buffer;
    int             offset, count, i, originalOffset;

    mprAssert(1 <= argc && argc <= 3);

    buffer = (EjsByteArray*) argv[0];
    offset = (argc == 2) ? ejsGetInt(argv[1]) : -1;
    count = (argc == 3) ? ejsGetInt(argv[2]) : buffer->length;

    if (count < 0) {
        count = buffer->length;
    }
    originalOffset = offset;
    if (offset < 0) {
        offset = buffer->writePosition;
    } else if (offset >= buffer->length) {
        offset = 0;
    }

    if (getInput(ejs, ap, 1) <= 0) {
        return (EjsVar*) ejs->zeroValue;
    }

    count = min(availableBytes(ap), count);
    for (i = 0; i < count; i++) {
        buffer->value[offset++] = ap->value[ap->readPosition++];
    }
    if (originalOffset < 0) {
        buffer->writePosition += count;
    }

    return (EjsVar*) ejsCreateNumber(ejs, count);
}


/*
 *  Read a boolean from the array.
 *
 *  function readBoolean(): Boolean
 */
static EjsVar *readBoolean(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     result;

    if (getInput(ejs, ap, 1) <= 0) {
        return 0;
    }
    result = ap->value[ap->readPosition];
    adjustReadPosition(ap, 1);

    return (EjsVar*) ejsCreateBoolean(ejs, result);
}


/*
 *  Read a byte from the array.
 *
 *  function readByte(): Number
 */
static EjsVar *readByte(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     result;

    if (getInput(ejs, ap, 1) <= 0) {
        return 0;
    }
    result = ap->value[ap->readPosition];
    adjustReadPosition(ap, 1);

    return (EjsVar*) ejsCreateNumber(ejs, result);
}


/**
 *  Read a date from the array.
 *
 *  function readDate(): Date
 */
static EjsVar *readDate(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    double  value;

    if (getInput(ejs, ap, EJS_SIZE_DOUBLE) <= 0) {
        return 0;
    }

    value = * (double*) &ap->value[ap->readPosition];
    value = swapDouble(ap, value);
    adjustReadPosition(ap, sizeof(double));

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) value);
}


#if BLD_FEATURE_FLOATING_POINT && ES_ByteArray_readDouble
/**
 *  Read a double from the array. The data will be decoded according to the encoding property.
 *
 *  function readDouble(): Date
 */
static EjsVar *readDouble(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    double  value;

    if (getInput(ejs, ap, EJS_SIZE_DOUBLE) <= 0) {
        return 0;
    }

    value = * (double*) &ap->value[ap->readPosition];
    value = swapDouble(ap, value);
    adjustReadPosition(ap, sizeof(double));

    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) value);
}
#endif

/*
 *  Read a 32-bit integer from the array. The data will be decoded according to the encoding property.
 *
 *  function readInteger(): Number
 */
static EjsVar *readInteger(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     value;

    if (getInput(ejs, ap, EJS_SIZE_INT) <= 0) {
        return 0;
    }

    value = * (int*) &ap->value[ap->readPosition];
    value = swap32(ap, value);
    adjustReadPosition(ap, sizeof(int));

    return (EjsVar*) ejsCreateNumber(ejs, value);
}


/*
 *  Read a 64-bit long from the array.The data will be decoded according to the encoding property.
 *
 *  function readLong(): Number
 */
static EjsVar *readLong(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int64   value;

    if (getInput(ejs, ap, EJS_SIZE_LONG) <= 0) {
        return 0;
    }

    value = * (int64*) &ap->value[ap->readPosition];
    value = swap64(ap, value);
    adjustReadPosition(ap, sizeof(int64));

    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) value);
}


/*
 *  Get the current read position offset
 *
 *  function get readPosition(): Number
 */
static EjsVar *readPosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->readPosition);
}


/*
 *  Set the current read position offset
 *
 *  function set readPosition(position: Number): Void
 */
static EjsVar *setReadPosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     pos;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    pos = ejsGetInt(argv[0]);
    if (pos < 0 || pos > ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad position value");
        return 0;
    }
    if (pos > ap->writePosition) {
        ejsThrowStateError(ejs, "Read position is greater than write position");
    } else {
        ap->readPosition = pos;
    }
    return 0;
}


/*
 *  Read a 16-bit short integer from the array. The data will be decoded according to the encoding property.
 *
 *  function readShort(): Number
 */
static EjsVar *readShort(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     value;

    if (getInput(ejs, ap, EJS_SIZE_SHORT) <= 0) {
        return 0;
    }

    value = * (short*) &ap->value[ap->readPosition];
    value = swap16(ap, value);
    adjustReadPosition(ap, sizeof(short));

    return (EjsVar*) ejsCreateNumber(ejs, value);
}


/*
 *  Read a UTF-8 string from the array. Read data from the read position up to the write position but not more than count
 *  characters.
 *
 *  function readString(count: Number = -1): String
 */
static EjsVar *baReadString(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsVar  *result;
    int     count;

    count = (argc == 1) ? ejsGetInt(argv[0]) : -1;

    if (count < 0) {
        if (getInput(ejs, ap, 1) < 0) {
            return 0;
        }
        count = availableBytes(ap);

    } else if (getInput(ejs, ap, count) < 0) {
        return 0;
    }

    count = min(count, availableBytes(ap));
    result = (EjsVar*) ejsCreateStringWithLength(ejs, (cchar*) &ap->value[ap->readPosition], count);
    adjustReadPosition(ap, count);

    return result;
}


#if ES_ByteArray_readXML && BLD_FEATURE_EJS_E4X
/*
 *  Read an XML document from the array.
 *
 *  function readXML(): XML
 */
static EjsVar *readXML(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsXML      *xml;

    //  TODO - does this work?
    if (getInput(ejs, ap, -1) <= 0) {
        return 0;
    }

    xml = ejsCreateXML(ejs, 0, 0, 0, 0);
    if (xml == 0) {
        mprAssert(ejs->exception);
        return 0;
    }

    //  TODO - need to make sure that the string is null terminated
    //  TODO - need a rc
    ejsLoadXMLString(ejs, xml, (cchar*) &ap->value[ap->readPosition]);

    //  TODO -
    adjustReadPosition(ap, 0);

    return (EjsVar*) xml;
}
#endif


/*
 *  Reset the read and write position pointers if there is no available data.
 *
 *  function reset(): Void
 */
static EjsVar *reset(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    if (ap->writePosition == ap->readPosition) {
        ap->writePosition = ap->readPosition = 0;
    }
    return 0;
}


/**
 *  Get the number of data bytes that the array can store from the write position till the end of the array.
 *
 *  function get room(): Number
 */
static EjsVar *roomProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length - ap->writePosition);
}


/*
 *  Convert the byte array data between the read and write positions into a string.
 *
 *  override function toString(): String
 */
static EjsVar *byteArrayToString(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateStringWithLength(ejs, (cchar*) &ap->value[ap->readPosition], availableBytes(ap));
}


/*
 *  Write data to the array. Data is written to the current write $position pointer.
 *
 *  function write(...data): Number
 */
EjsNumber *ejsWriteToByteArray(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsArray        *args;
    EjsByteArray    *bp;
    EjsString       *sp;
    EjsVar          *vp;
    int             i, start, len;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    /*
     *  Unwrap nested arrays
     */
    args = (EjsArray*) argv[0];
    while (args && ejsIsArray(args) && args->length == 1) {
        vp = ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (!ejsIsArray(vp)) {
            break;
        }
        args = (EjsArray*) vp;
    }

    start = ap->writePosition;

    for (i = 0; i < args->length; i++) {

        vp = ejsGetProperty(ejs, (EjsVar*) args, i);
        if (vp == 0) {
            continue;
        }

        switch (vp->type->id) {

        case ES_Boolean:
            if (!makeRoom(ejs, ap, EJS_SIZE_BOOLEAN)) {
                return 0;
            }
            putByte(ap, ejsGetBoolean(vp));
            break;

        case ES_Date:
            if (!makeRoom(ejs, ap, EJS_SIZE_DOUBLE)) {
                return 0;
            }
            putNumber(ap, (MprNumber) ((EjsDate*) vp)->value);
            break;

        case ES_Number:
            if (!makeRoom(ejs, ap, EJS_SIZE_DOUBLE)) {
                return 0;
            }
            putNumber(ap, ejsGetNumber(vp));
            break;

        case ES_String:
            if (!makeRoom(ejs, ap, ((EjsString*) vp)->length)) {
                return 0;
            }
            sp = (EjsString*) vp;
            putString(ap, sp->value, sp->length);
            break;

        default:
            sp = ejsToString(ejs, vp);
            putString(ap, sp->value, sp->length);
            break;

        case ES_ByteArray:
            bp = (EjsByteArray*) vp;
            len = availableBytes(bp);
            if (!makeRoom(ejs, ap, len)) {
                return 0;
            }
            /*
             *  Note: this only copies between the read/write positions of the source byte array
             */
            ejsCopyToByteArray(ejs, ap, ap->writePosition, (char*) &bp->value[bp->readPosition], len);
            ap->writePosition += len;
            break;
        }
    }
    return ejsCreateNumber(ejs, ap->writePosition - start);
}


/*
 *  Write a byte to the array
 *
 *  function writeByte(value: Number): Void
 */
static EjsVar *writeByte(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, 1)) {
        return 0;
    }
    putByte(ap, ejsGetInt(argv[0]));
    return 0;
}


/*
 *  Write a short to the array
 *
 *  function writeShort(value: Number): Void
 */
static EjsVar *writeShort(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(short))) {
        return 0;
    }
    putShort(ap, ejsGetInt(argv[0]));
    return 0;
}


#if ES_ByteArray_writeDouble && BLD_FEATURE_FLOATING_POINT
/*
 *  Write a double to the array
 *
 *  function writeDouble(value: Number): Void
 */
static EjsVar *writeDouble(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(double))) {
        return 0;
    }
    putDouble(ap, ejsGetDouble(argv[0]));
    return 0;
}
#endif


/*
 *  Write an integer (32 bits) to the array
 *
 *  function writeInteger(value: Number): Void
 */

static EjsVar *writeInteger(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(int))) {
        return 0;
    }
    putInteger(ap, ejsGetInt(argv[0]));
    return 0;
}


/*
 *  Write a long (64 bit) to the array
 *
 *  function writeLong(value: Number): Void
 */
static EjsVar *writeLong(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(int))) {
        return 0;
    }
    putLong(ap, ejsGetInt(argv[0]));
    return 0;
}


/*
 *  Get the current write position offset
 *
 *  function get writePosition(): Number
 */
static EjsVar *writePosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->writePosition);
}


/*
 *  Set the current write position offset
 *
 *  function set writePosition(position: Number): Void
 */
static EjsVar *setWritePosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     pos;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    pos = ejsGetInt(argv[0]);
    if (pos < 0 || pos >= ap->length) {
        pos = 0;
    }
    if (pos < ap->readPosition) {
        ejsThrowStateError(ejs, "Write position is less than read position");
    } else {
        ap->writePosition = pos;
    }
    return 0;
}


/*
 *  Flush the array. If an output function is defined, invoke it to accept the data.
 */
static int flushByteArray(Ejs *ejs, EjsByteArray *ap)
{
    EjsVar      *arg;

    if (ap->output == 0) {
        return 0;
    }

    while (availableBytes(ap) && !ejs->exception) {
        arg = (EjsVar*) ap;
        ejsRunFunction(ejs, ap->output, (EjsVar*) ap, 1, &arg);
    }
    ap->writePosition = ap->readPosition = 0;

    return 0;
}


static int growByteArray(Ejs *ejs, EjsByteArray *ap, int len)
{
    if (len > ap->length) {
        ap->value = mprRealloc(ap, ap->value, len * sizeof(char*));
        if (ap->value == 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }
        memset(&ap->value[ap->length], 0, len - ap->length);
        ap->growInc = min(ap->growInc * 2, 32 * 1024);
        ap->length = len;
    }
    return 0;
}


/*
 *  Get more input, sufficient to satisfy the rquired number of bytes. The required parameter specifies how many bytes MUST be 
 *  read. Short fills are not permitted. Return the count of bytes available or 0 if the required number of bytes can't be read. 
 *  Return -ve on errors.
 */
static int getInput(Ejs *ejs, EjsByteArray *ap, int required)
{
    EjsVar      *argv[3];
    int         lastPos;

    if (availableBytes(ap) == 0) {
        ap->writePosition = ap->readPosition = 0;
    }
    if (ap->input) {
        while (availableBytes(ap) < required && !ejs->exception) {
            lastPos = ap->writePosition;

            /*
             *  Run the input function. Get as much data as possible without blocking.
             *  The input function will write to the byte array and will update the writePosition.
             */
            argv[0] = (EjsVar*) ap;
            ejsRunFunction(ejs, ap->input, (EjsVar*) ap, 1, argv);
            if (lastPos == ap->writePosition) {
                break;
            }
#if UNUSED
            if (!ejsIsNumber(result)) {
                if (ejs->exception == NULL) {
                    ejsThrowIOError(ejs, "input callback did not return a number");
                }
                return EJS_ERR;
            }
            count = ejsGetInt(result);
            if (count < 0) {
                ejsThrowIOError(ejs, "Can't read data");
                return EJS_ERR;

            } else if (count == 0) {
                return 0;
            }
#endif
        }
    }
    if (availableBytes(ap) < required) {
        return 0;
    }
    return availableBytes(ap);
}


static bool makeRoom(Ejs *ejs, EjsByteArray *ap, int require)
{
    int     newLen;

    if (room(ap) < require) {
        if (flushByteArray(ejs, ap) < 0) {
            return 0;
        }
        newLen = max(ap->length + require, ap->length + ap->growInc);
        if (!ap->growable || growByteArray(ejs, ap, newLen) < 0) {
            if (ejs->exception == NULL) {
                ejsThrowResourceError(ejs, "Byte array is too small");
            }
            return 0;
        }
    }
    return 1;
}


static MPR_INLINE int swap16(EjsByteArray *ap, int a)
{
    if (!ap->swap) {
        return a;
    }
    return (a & 0xFF) << 8 | (a & 0xFF00 >> 8);
}


static MPR_INLINE int swap32(EjsByteArray *ap, int a)
{
    if (!ap->swap) {
        return a;
    }
    return (a & 0xFF) << 24 | (a & 0xFF00 << 8) | (a & 0xFF0000 >> 8) | (a & 0xFF000000 >> 16);
}


static MPR_INLINE int64 swap64(EjsByteArray *ap, int64 a)
{
    int64   low, high;

    if (!ap->swap) {
        return a;
    }

    low = a & 0xFFFFFFFF;
    high = (a >> 32) & 0xFFFFFFFF;

    return  (low & 0xFF) << 24 | (low & 0xFF00 << 8) | (low & 0xFF0000 >> 8) | (low & 0xFF000000 >> 16) |
            ((high & 0xFF) << 24 | (high & 0xFF00 << 8) | (high & 0xFF0000 >> 8) | (high & 0xFF000000 >> 16)) << 32;
}


static MPR_INLINE double swapDouble(EjsByteArray *ap, double a)
{
    int64   low, high;

    if (!ap->swap) {
        return a;
    }

    low = ((int64) a) & 0xFFFFFFFF;
    high = (((int64) a) >> 32) & 0xFFFFFFFF;

    return  (double) ((low & 0xFF) << 24 | (low & 0xFF00 << 8) | (low & 0xFF0000 >> 8) | (low & 0xFF000000 >> 16) |
            ((high & 0xFF) << 24 | (high & 0xFF00 << 8) | (high & 0xFF0000 >> 8) | (high & 0xFF000000 >> 16)) << 32);
}


static void putByte(EjsByteArray *ap, int value)
{
    ap->value[ap->writePosition++] = (char) value;
}


static void putShort(EjsByteArray *ap, int value)
{
    value = swap16(ap, value);

    *((short*) &ap->value[ap->writePosition]) = (short) value;
    ap->writePosition += sizeof(short);
}


static void putInteger(EjsByteArray *ap, int value)
{
    value = swap32(ap, value);

    *((int*) &ap->value[ap->writePosition]) = (int) value;
    ap->writePosition += sizeof(int);
}


static void putLong(EjsByteArray *ap, int64 value)
{
    value = swap64(ap, value);

    *((int64*) &ap->value[ap->writePosition]) = value;
    ap->writePosition += sizeof(int64);
}


#if BLD_FEATURE_FLOATING_POINT
static void putDouble(EjsByteArray *ap, double value)
{
    value = swapDouble(ap, value);

    *((double*) &ap->value[ap->writePosition]) = value;
    ap->writePosition += sizeof(double);
}
#endif


/*
 *  Write a number in the default number encoding
 */
static void putNumber(EjsByteArray *ap, MprNumber value)
{
#if BLD_FEATURE_FLOATING_POINT
    putDouble(ap, value);
#elif MPR_64_BIT
    putLong(ap, value);
#else
    putInteger(ap, value);
#endif
}


static void putString(EjsByteArray *ap, cchar *value, int len)
{
    mprMemcpy(&ap->value[ap->writePosition], room(ap), value, len);
    ap->writePosition += len;
}


void ejsSetByteArrayPositions(Ejs *ejs, EjsByteArray *ap, int readPosition, int writePosition)
{
    //  TODO - should validate the positions here against valid limits
    if (readPosition >= 0) {
        ap->readPosition = readPosition;
    }
    if (writePosition >= 0) {
        ap->writePosition = writePosition;
    }
}


//  TODO - should return a count byte copied
int ejsCopyToByteArray(Ejs *ejs, EjsByteArray *ap, int offset, char *data, int length)
{
    int     i;

    mprAssert(ap);
    mprAssert(data);

    if (!makeRoom(ejs, ap, offset + length)) {
        return EJS_ERR;
    }

    if (ap->length < (offset + length)) {
        return EJS_ERR;
    }

    for (i = 0; i < ap->length; i++) {
        ap->value[offset++] = data[i];
    }
    return 0;
}


int ejsGetAvailableData(EjsByteArray *ap)
{
    return availableBytes(ap);
}


EjsByteArray *ejsCreateByteArray(Ejs *ejs, int size)
{
    EjsByteArray    *ap;

    /*
     *  No need to invoke constructor
     */
    ap = (EjsByteArray*) ejsCreateVar(ejs, ejs->byteArrayType, 0);
    if (ap == 0) {
        return 0;
    }

    if (size <= 0) {
        size = MPR_BUFSIZE;
    }

    if (growByteArray(ejs, ap, size) < 0) {
        return 0;
    }
    ap->length = size;
    ap->growable = 1;
    ap->growInc = MPR_BUFSIZE;
    ap->endian = mprGetEndian(ejs);

    ejsSetDebugName(ap, "ByteArray instance");

    return ap;
}


void ejsCreateByteArrayType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "ByteArray"), ejs->objectType, sizeof(EjsByteArray),
        ES_ByteArray, ES_ByteArray_NUM_CLASS_PROP, ES_ByteArray_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->byteArrayType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castByteArrayVar;
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneByteArrayVar;
    type->helpers->getProperty = (EjsGetPropertyHelper) getByteArrayProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getByteArrayPropertyCount;
    type->helpers->deleteProperty = (EjsDeletePropertyHelper) deleteByteArrayProperty;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeByteArrayOperator;
    type->helpers->markVar = (EjsMarkVarHelper) markByteArrayVar;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupByteArrayProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setByteArrayProperty;
}


void ejsConfigureByteArrayType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->byteArrayType;
    
    ejsBindMethod(ejs, type, ES_ByteArray_ByteArray, (EjsNativeFunction) byteArrayConstructor);
    ejsBindMethod(ejs, type, ES_ByteArray_available, (EjsNativeFunction) availableProc);
    ejsBindMethod(ejs, type, ES_ByteArray_copyIn, (EjsNativeFunction) copyIn);
    ejsBindMethod(ejs, type, ES_ByteArray_copyOut, (EjsNativeFunction) copyOut);
    ejsBindMethod(ejs, type, ES_ByteArray_set_input, (EjsNativeFunction) inputByteArrayData);
    ejsBindMethod(ejs, type, ES_ByteArray_flush, (EjsNativeFunction) flushProc);
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) getByteArrayLength);
    ejsBindMethod(ejs, type, ES_Object_get, (EjsNativeFunction) getByteArrayIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, (EjsNativeFunction) getByteArrayValues);
    ejsBindMethod(ejs, type, ES_ByteArray_endian, (EjsNativeFunction) endian);
    ejsBindMethod(ejs, type, ES_ByteArray_set_endian, (EjsNativeFunction) setEndian);
    ejsBindMethod(ejs, type, ES_ByteArray_set_output, (EjsNativeFunction) output);
    ejsBindMethod(ejs, type, ES_ByteArray_read, (EjsNativeFunction) byteArrayReadProc);
    ejsBindMethod(ejs, type, ES_ByteArray_readBoolean, (EjsNativeFunction) readBoolean);
    ejsBindMethod(ejs, type, ES_ByteArray_readByte, (EjsNativeFunction) readByte);
    ejsBindMethod(ejs, type, ES_ByteArray_readDate, (EjsNativeFunction) readDate);
#if BLD_FEATURE_FLOATING_POINT && ES_ByteArray_readDouble
    ejsBindMethod(ejs, type, ES_ByteArray_readDouble, (EjsNativeFunction) readDouble);
#endif
    ejsBindMethod(ejs, type, ES_ByteArray_readInteger, (EjsNativeFunction) readInteger);
    ejsBindMethod(ejs, type, ES_ByteArray_readLong, (EjsNativeFunction) readLong);
    ejsBindMethod(ejs, type, ES_ByteArray_readPosition, (EjsNativeFunction) readPosition);
    ejsBindMethod(ejs, type, ES_ByteArray_set_readPosition, (EjsNativeFunction) setReadPosition);
    ejsBindMethod(ejs, type, ES_ByteArray_readShort, (EjsNativeFunction) readShort);
    ejsBindMethod(ejs, type, ES_ByteArray_readString, (EjsNativeFunction) baReadString);
#if ES_ByteArray_readXML && BLD_FEATURE_EJS_E4X
    ejsBindMethod(ejs, type, ES_ByteArray_readXML, (EjsNativeFunction) readXML);
#endif
    ejsBindMethod(ejs, type, ES_ByteArray_reset, (EjsNativeFunction) reset);
    ejsBindMethod(ejs, type, ES_ByteArray_room, (EjsNativeFunction) roomProc);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) byteArrayToString);
    ejsBindMethod(ejs, type, ES_ByteArray_write, (EjsNativeFunction) ejsWriteToByteArray);
    ejsBindMethod(ejs, type, ES_ByteArray_writeByte, (EjsNativeFunction) writeByte);
    ejsBindMethod(ejs, type, ES_ByteArray_writeShort, (EjsNativeFunction) writeShort);
    ejsBindMethod(ejs, type, ES_ByteArray_writeInteger, (EjsNativeFunction) writeInteger);
    ejsBindMethod(ejs, type, ES_ByteArray_writeLong, (EjsNativeFunction) writeLong);
#if ES_ByteArray_writeDouble && BLD_FEATURE_FLOATING_POINT
    ejsBindMethod(ejs, type, ES_ByteArray_writeDouble, (EjsNativeFunction) writeDouble);
#endif
    ejsBindMethod(ejs, type, ES_ByteArray_writePosition, (EjsNativeFunction) writePosition);
    ejsBindMethod(ejs, type, ES_ByteArray_set_writePosition, (EjsNativeFunction) setWritePosition);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsByteArray.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsDate.c"
 */
/************************************************************************/

/**
 *  ejsDate.c - Date type class
 *
 *  Date/time is store internally as milliseconds since 1970/01/01 GMT
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  TODO - move to ejsNumber.h. But would have to rename fixed() to ejsFixed()
 */
#if BLD_FEATURE_FLOATING_POINT
#define fixed(n) ((int64) (floor(n)))
#else
#define fixed(n) (n)
#endif

#if WIN
#pragma warning (disable:4244)
#endif

/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castDate(Ejs *ejs, EjsDate *dp, EjsType *type)
{
    struct tm   tm;
    char        buf[80];

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) dp->value);

    case ES_String:
        /*
         *  Format:  Tue Jul 15 2009 10:53:23 GMT-0700 (PDT)
         */
        mprLocaltime(ejs, &tm, dp->value);
        mprStrftime(ejs, buf, sizeof(buf), "%a %b %d %Y %T GMT%z (%Z)", &tm);
        return (EjsVar*) ejsCreateString(ejs, buf);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }

    return 0;
}



/*
 *  TODO - this is the same as number. Should share code
 */
static EjsVar *coerceDateOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->nanValue;
        } else if (ejsIsNull(rhs)) {
            rhs = (EjsVar*) ejs->zeroValue;
        } else if (ejsIsBoolean(rhs) || ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        } else {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) (((EjsDate*) lhs)->value ? ejs->trueValue : ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsDate*) lhs)->value ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }

    return 0;
}



static EjsVar *invokeDateOperator(Ejs *ejs, EjsDate *lhs, int opcode, EjsDate *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceDateOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }

    switch (opcode) {
    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value == rhs->value);

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs->value == rhs->value));

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value < rhs->value);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value <= rhs->value);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value > rhs->value);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value >= rhs->value);

    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_NEG:
        return (EjsVar*) ejsCreateNumber(ejs, - (MprNumber) lhs->value);

    case EJS_OP_LOGICAL_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, (MprNumber) !fixed(lhs->value));

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (~fixed(lhs->value)));

    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return (EjsVar*) ejsCreateDate(ejs, lhs->value + rhs->value);

    case EJS_OP_AND:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) & fixed(rhs->value)));

    case EJS_OP_DIV:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
#endif
        return (EjsVar*) ejsCreateDate(ejs, lhs->value / rhs->value);

    case EJS_OP_MUL:
        return (EjsVar*) ejsCreateDate(ejs, lhs->value * rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) | fixed(rhs->value)));

    case EJS_OP_REM:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
#endif
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) % fixed(rhs->value)));

    case EJS_OP_SHL:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) << fixed(rhs->value)));

    case EJS_OP_SHR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_SUB:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) - fixed(rhs->value)));

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) ^ fixed(rhs->value)));

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
    /* Should never get here */
}

/*
 *  TODO Date constructor
 *
 *      Date()
 *      Date(milliseconds)
 *      Date(dateString)
 *      Date(year, month, date)
 *      Date(year, month, date, hour, minute, second)
 */

static EjsVar *dateConstructor(Ejs *ejs, EjsDate *date, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      *vp;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    switch (args->length) {
    case 0:
        /* Now */
        date->value = mprGetTime(ejs);
        break;

    case 1:
        vp = ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (ejsIsNumber(vp)) {
            /* Milliseconds */
            date->value = ejsGetNumber(vp);

        } else if (ejsIsString(vp)) {
            if (mprParseTime(ejs, &date->value, ejsGetString(vp)) < 0) {
                ejsThrowArgError(ejs, "Can't parse date string: %s", ejsGetString(vp));
                return 0;
            }
        } else if (ejsIsDate(vp)) {
            date->value = ((EjsDate*) vp)->value;

        } else {
            ejsThrowArgError(ejs, "Can't construct date from this argument");
        }
        break;

    //  TODO - must do other args

    default:
        ejsThrowArgError(ejs, "Too many arguments");
    }

    return (EjsVar*) date;
}




static EjsVar *getDay(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_wday);
}



/*
 *  Return day of year (0 - 366)
 */
static EjsVar *getDayOfYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_yday);
}



/*
 *  Return day of month (0-31)
 */
static EjsVar *getDate(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_mday);
}



/*
 *  Get the elapsed time in milliseconds since the Date object was constructed
 */
static EjsVar *elapsed(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetElapsedTime(ejs, dp->value));
}



/*
 *  Return year in 4 digits
 */
static EjsVar *getFullYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_year + 1900);
}



/*
 *  Update the year component using a 4 digit year
 */
static EjsVar *setFullYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_yday  = ejsGetNumber(argv[0]) - 1900;
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



/*
 *  function format(layout: String): String
 */
static EjsVar *formatDate(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    char        buf[80];
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    mprStrftime(ejs, buf, sizeof(buf), ejsGetString(argv[0]), &tm);
    return (EjsVar*) ejsCreateString(ejs, buf);
}



/*
 *  Return hour of day (0-23)
 */
static EjsVar *getHours(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_hour);
}



/*
 *  Update the hour of the day using a 0-23 hour
 */
static EjsVar *setHours(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_hour = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



static EjsVar *getMilliseconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ((int64) dp->value) % 1000);
}



static EjsVar *setMilliseconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ((int) dp->value) % 1000);
}



static EjsVar *getMinutes(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_min);
}



static EjsVar *setMinutes(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_min = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



/*
 *  Get the month (0-11)
 */
static EjsVar *getMonth(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_mon);
}



static EjsVar *setMonth(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_mon = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



static EjsVar *nextDay(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    int64       inc;

    if (argc == 1) {
        inc = ejsGetNumber(argv[0]);
    } else {
        inc = 1;
    }
    return (EjsVar*) ejsCreateDate(ejs, dp->value + (inc * 86400 * 1000));
}



static EjsVar *now(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetTime(ejs));
}



static EjsVar *parseDate(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    MprTime     when;

    if (mprParseTime(ejs, &when, ejsGetString(argv[0])) < 0) {
        ejsThrowArgError(ejs, "Can't parse date string: %s", ejsGetString(argv[0]));
        return 0;
    }
    return (EjsVar*) ejsCreateDate(ejs, when);
}



/*
 *  Get seconds (0-60)
 */
static EjsVar *getSeconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_sec);
}



static EjsVar *setSeconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_sec = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



static EjsVar *getTime(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, dp->value);
}



static EjsVar *setTime(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    dp->value = ejsGetNumber(argv[0]);
    return 0;
}



static EjsVar *dateToString(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return castDate(ejs, dp, ejs->stringType);
}



static EjsVar *getYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_year + 1900);
}


static EjsVar *setYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_year = ejsGetNumber(argv[0]) - 1900;
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}


/*
 *  Create an initialized date object. Set to the current time if value is zero.
 */

EjsDate *ejsCreateDate(Ejs *ejs, MprTime value)
{
    EjsDate *vp;

    vp = (EjsDate*) ejsCreateVar(ejs, ejs->dateType, 0);
    if (vp != 0) {
        vp->value = value;
    }
    return vp;
}


void ejsCreateDateType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Date"), ejs->objectType, sizeof(EjsDate),
        ES_Date, ES_Date_NUM_CLASS_PROP, ES_Date_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->dateType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castDate;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeDateOperator;
}


void ejsConfigureDateType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->dateType;

    ejsBindMethod(ejs, type, ES_Date_Date, (EjsNativeFunction) dateConstructor);
    ejsBindMethod(ejs, type, ES_Date_day, (EjsNativeFunction) getDay);
    ejsBindMethod(ejs, type, ES_Date_dayOfYear, (EjsNativeFunction) getDayOfYear);
    ejsBindMethod(ejs, type, ES_Date_date, (EjsNativeFunction) getDate);
    ejsBindMethod(ejs, type, ES_Date_fullYear, (EjsNativeFunction) getFullYear);
    ejsBindMethod(ejs, type, ES_Date_set_fullYear, (EjsNativeFunction) setFullYear);
    ejsBindMethod(ejs, type, ES_Date_elapsed, (EjsNativeFunction) elapsed);

    ejsBindMethod(ejs, type, ES_Date_format, (EjsNativeFunction) formatDate);

#if ES_Date_getDay
    ejsBindMethod(ejs, type, ES_Date_getDay, (EjsNativeFunction) getDay);
#endif
#if ES_Date_getDate
    ejsBindMethod(ejs, type, ES_Date_getDate, (EjsNativeFunction) getDate);
#endif
#if ES_Date_getFullYear
    ejsBindMethod(ejs, type, ES_Date_getFullYear, (EjsNativeFunction) getFullYear);
#endif
#if ES_Date_getHours
    ejsBindMethod(ejs, type, ES_Date_getHours, (EjsNativeFunction) getHours);
#endif
#if ES_Date_getMilliseconds
    ejsBindMethod(ejs, type, ES_Date_getMilliseconds, (EjsNativeFunction) getMilliseconds);
#endif
#if ES_Date_getMinutes
    ejsBindMethod(ejs, type, ES_Date_getMinutes, (EjsNativeFunction) getMinutes);
#endif
#if ES_Date_getMonth
    ejsBindMethod(ejs, type, ES_Date_getMonth, (EjsNativeFunction) getMonth);
#endif
#if ES_Date_getSeconds
    ejsBindMethod(ejs, type, ES_Date_getSeconds, (EjsNativeFunction) getSeconds);
#endif
#if ES_Date_getTime
    ejsBindMethod(ejs, type, ES_Date_getTime, (EjsNativeFunction) getTime);
#endif
#if ES_Date_getUTCMonth
    ejsBindMethod(ejs, type, ES_Date_getUTCMonth, (EjsNativeFunction) getUTCMonth);
#endif
#if ES_Date_getTimezoneOffset
    ejsBindMethod(ejs, type, ES_Date_getTimezoneOffset, (EjsNativeFunction) getTimezoneOffset);
#endif

    ejsBindMethod(ejs, type, ES_Date_hours, (EjsNativeFunction) getHours);
    ejsBindMethod(ejs, type, ES_Date_set_hours, (EjsNativeFunction) setHours);
    ejsBindMethod(ejs, type, ES_Date_milliseconds, (EjsNativeFunction) getMilliseconds);
    ejsBindMethod(ejs, type, ES_Date_set_milliseconds, (EjsNativeFunction) setMilliseconds);
    ejsBindMethod(ejs, type, ES_Date_minutes, (EjsNativeFunction) getMinutes);
    ejsBindMethod(ejs, type, ES_Date_set_minutes, (EjsNativeFunction) setMinutes);
    ejsBindMethod(ejs, type, ES_Date_month, (EjsNativeFunction) getMonth);
    ejsBindMethod(ejs, type, ES_Date_set_month, (EjsNativeFunction) setMonth);
    ejsBindMethod(ejs, type, ES_Date_nextDay, (EjsNativeFunction) nextDay);
    ejsBindMethod(ejs, type, ES_Date_now, (EjsNativeFunction) now);
    ejsBindMethod(ejs, type, ES_Date_parseDate, (EjsNativeFunction) parseDate);

#if ES_Date_parse
    ejsBindMethod(ejs, type, ES_Date_parse, (EjsNativeFunction) parse);
#endif

    ejsBindMethod(ejs, type, ES_Date_seconds, (EjsNativeFunction) getSeconds);
    ejsBindMethod(ejs, type, ES_Date_set_seconds, (EjsNativeFunction) setSeconds);

#if ES_Date_setFullYear
    ejsBindMethod(ejs, type, ES_Date_setFullYear, (EjsNativeFunction) setFullYear);
#endif
#if ES_Date_setMilliseconds
    ejsBindMethod(ejs, type, ES_Date_setMilliseconds, (EjsNativeFunction) setMilliseconds);
#endif
#if ES_Date_setMinutes
    ejsBindMethod(ejs, type, ES_Date_setMinutes, (EjsNativeFunction) setMinutes);
#endif
#if ES_Date_setMonth
    ejsBindMethod(ejs, type, ES_Date_setMonth, (EjsNativeFunction) setMonth);
#endif
#if ES_Date_setSeconds
    ejsBindMethod(ejs, type, ES_Date_setSeconds, (EjsNativeFunction) setSeconds);
#endif
#if ES_Date_setTime
    ejsBindMethod(ejs, type, ES_Date_setTime, (EjsNativeFunction) setTime);
#endif

    ejsBindMethod(ejs, type, ES_Date_time, (EjsNativeFunction) getTime);
    ejsBindMethod(ejs, type, ES_Date_set_time, (EjsNativeFunction) setTime);

#if ES_Date_toDateString
    ejsBindMethod(ejs, type, ES_Date_toDateString, (EjsNativeFunction) toDateString);
#endif
#if ES_Date_toIOString
    ejsBindMethod(ejs, type, ES_Date_toIOString, (EjsNativeFunction) toIOString);
#endif
#if ES_Date_toJSONString
    ejsBindMethod(ejs, type, ES_Date_toJSONString, (EjsNativeFunction) toJSONString);
#endif
#if ES_Date_toLocaleDateString
    ejsBindMethod(ejs, type, ES_Date_toLocaleDateString, (EjsNativeFunction) toLocaleDateString);
#endif
#if ES_Date_toLocaleString
    ejsBindMethod(ejs, type, ES_Date_toLocaleString, (EjsNativeFunction) toLocaleString);
#endif
#if ES_Date_toLocaleTimeString
    ejsBindMethod(ejs, type, ES_Date_toLocaleTimeString, (EjsNativeFunction) toLocaleTimeString);
#endif
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) dateToString);
#if ES_Date_toTimeString
    ejsBindMethod(ejs, type, ES_Date_toTimeString, (EjsNativeFunction) toTimeString);
#endif
#if ES_Date_toUTCString
    ejsBindMethod(ejs, type, ES_Date_toUTCString, (EjsNativeFunction) toUTCString);
#endif
#if ES_Date_UTC
    ejsBindMethod(ejs, type, ES_Date_UTC, (EjsNativeFunction) UTC);
#endif
#if ES_Date_UTCmonth
    ejsBindMethod(ejs, type, ES_Date_UTCmonth, (EjsNativeFunction) UTCmonth);
#endif

#if ES_Date_valueOf
    ejsBindMethod(ejs, type, ES_Date_valueOf, (EjsNativeFunction) valueOf);
#endif
    ejsBindMethod(ejs, type, ES_Date_year, (EjsNativeFunction) getYear);
    ejsBindMethod(ejs, type, ES_Date_set_year, (EjsNativeFunction) setYear);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsDate.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsDecimal.c"
 */
/************************************************************************/

/**
 *  ejsDecimal.c - Ejscript Decimal class
 *
 *  TODO - This class is not finished yet.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  Select 128 bit decimals
 */
#define     DECNUMDIGITS    38


#if BLD_FEATURE_DECIMAL
/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castDecimal(Ejs *ejs, EjsDecimal *vp, EjsType *type)
{
    char        numBuf[16];

    switch (type->primitiveType) {
    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;

#if FUTURE
    case EJS_TYPE_BOOLEAN:
        //  TODO - not complete
        mprAssert(0);
        return (EjsVar*) ejsCreateBoolean(ejs, (vp->value != 0) ? 1 : 0);

    case EJS_TYPE_DECIMAL:
        return vp;

#if BLD_FEATURE_FLOATING_POINT
    case EJS_TYPE_DOUBLE:
        return (EjsVar*) ejsCreateDouble(ejs, (double) vp->value);
#endif

    case EJS_TYPE_INTEGER:
        return (EjsVar*) vp;

    case EJS_TYPE_LONG:
        return (EjsVar*) ejsCreateLong(ejs, (int64) vp->value);

    case EJS_TYPE_NUMBER:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) vp->value);
#endif
        
    case EJS_TYPE_STRING:
        decNumberToString(&vp->value, numBuf);
        return (EjsVar*) ejsCreateString(ejs, numBuf);
    }

    ejsThrowTypeError(ejs, "Unknown type");
    return 0;
}



static EjsVar *invokeDecimalOperator(Ejs *ejs, EjsDecimal *lhs, int opCode, EjsDecimal *rhs)
{
    MprDecimal  result;
    decContext  *context;

    mprAssert(ejsIsDecimal(lhs));
    mprAssert(rhs == 0 || ejsIsDecimal(rhs));

    context = (decContext*) lhs->var.type->typeData;

    switch (opCode) {
    case EJS_OP_MUL:
        decNumberMultiply(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_DIV:
        decNumberDivide(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_REM:
        decNumberRemainder(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_ADD:
        decNumberAdd(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_SUB:
        decNumberSubtract(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

#if 0
    case EJS_OP_AND:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value & rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value | rhs->value);

    case EJS_OP_SHL:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value << rhs->value);

    case EJS_OP_SHR:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value >> rhs->value);

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateDecimal(ejs, 
            (uint) lhs->value >> rhs->value);

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value ^ rhs->value);

    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LE:
    case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE:
    case EJS_OP_COMPARE_GT:
        boolResult = 0;
        if (ejsIsNan(lhs->value) || ejsIsNan(rhs->value)) {
            boolResult = 0;

        } else if (lhs->value == rhs->value) {
            boolResult = 1;

        } else if (lhs->value == +0.0 && rhs->value == -0.0) {
            boolResult = 1;

        } else if (lhs->value == -0.0 && rhs->value == +0.0) {
            boolResult = 1;

        } else {

            switch (opCode) {
            case EJS_OP_COMPARE_EQ:
            case EJS_OP_COMPARE_STRICTLY_EQ:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value == rhs->value);

            case EJS_OP_COMPARE_NE:
            case EJS_OP_COMPARE_STRICTLY_NE:
                return (EjsVar*) ejsCreateBoolean(ejs, !(lhs->value == rhs->value));

            case EJS_OP_COMPARE_LE:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value <= rhs->value);

            case EJS_OP_COMPARE_LT:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value < rhs->value);

            case EJS_OP_COMPARE_GE:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value >= rhs->value);

            case EJS_OP_COMPARE_GT:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value > rhs->value);

            default:
                mprAssert(0);
            }
        }
        return (EjsVar*) ejsCreateBoolean(ejs, boolResult);

    case EJS_OP_INC:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value + rhs->value);

    case EJS_OP_NEG:
        mprAssert(0);
        return (EjsVar*) ejsCreateDecimal(ejs, -lhs->value);

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateDecimal(ejs, ~lhs->value);

#endif

    default:
        ejsThrowTypeError(ejs, "Operation not valid on this type");
        return 0;
    }
    /* Should never get here */
}



MprDecimal ejsParseDecimal(Ejs *ejs, cchar *str)
{
    EjsType     *type;
    MprDecimal  d;

    type = ejs->decimalType;
    mprAssert(type);

    decNumberFromString(&d, str, (decContext*) type->typeData);

    return d;
}



EjsDecimal *ejsCreateDecimal(Ejs *ejs, MprDecimal value)
{
    EjsDecimal  *vp;

    vp = (EjsDecimal*) ejsCreateVar(ejs, ejs->decimalType, 0);
    if (vp != 0) {
        vp->value = value;
    }
    return vp;
}



#if BLD_FEATURE_FLOATING_POINT
EjsDecimal *ejsCreateDecimalFromDouble(Ejs *ejs, double value)
{
    EjsType     *type;
    EjsDecimal  *vp;
    char        buf[16];

    type = ejs->decimalType;
    mprAssert(type);

    vp = (EjsDecimal*) ejsCreateVar(ejs, type, 0);
    if (vp != 0) {
        /*
         *  FUTURE -- must get ability to convert to decimal from other 
         *  than string!
         */
        mprSprintf(buf, sizeof(buf), "%f", value);
        decNumberFromString(&vp->value, buf, type->typeData);
    }
    return vp;
}
#endif



EjsDecimal *ejsCreateDecimalFromInteger(Ejs *ejs, int value)
{
    EjsType     *type;
    EjsDecimal  *vp;
    char        buf[16];

    type = ejs->decimalType;
    mprAssert(type);

    vp = (EjsDecimal*) ejsCreateVar(ejs, type, 0);
    if (vp != 0) {
        /*
         *  FUTURE -- must get ability to convert to decimal from other 
         *  than string!
         */
        mprItoa(buf, sizeof(buf), value);
        decNumberFromString(&vp->value, buf, type->typeData);
    }
    return vp;
}



EjsDecimal *ejsCreateDecimalFromLong(Ejs *ejs, int64 value)
{
    EjsType     *type;
    EjsDecimal  *vp;
    char        buf[16];

    type = ejs->decimalType;
    mprAssert(type);

    vp = (EjsDecimal*) ejsCreateVar(ejs, type, 0);
    if (vp != 0) {
        /*
         *  FUTURE -- must get ability to convert to decimal from other 
         *  than string!
         */
        mprSprintf(buf, sizeof(buf), "%Ld", value);
        decNumberFromString(&vp->value, buf, type->typeData);
    }
    return vp;
}



void ejsCreateDecimalType(Ejs *ejs)
{
    EjsType     *type;
    decContext  *context;                               // working context

    /*
     *  Allocate a decimal working context
     */
    context = mprAlloc(ejs, sizeof(decContext));
    type = ejsCreateIntrinsicType(ejs, "Decimal", 0, sizeof(EjsDecimal), 
        EJSLOT_Decimal_NUM_CLASS_PROP, EJSLOT_Decimal_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castDecimal;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeDecimalOperator;

    /*
     *  Initalize with no traps and define the precision
     */
    decContextDefault(context, DEC_INIT_BASE); 
    context->traps = 0;
    context->digits = DECNUMDIGITS;
    type->typeData = context;

    ejs->decimalType = type;
    ejsAddCoreType(ejs, EJS_TYPE_DECIMAL, type);
}



void ejsConfigureDecimalType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->decimalType;
    mprAssert(type);
    
    /*
     *  Define the "decimal" alias
     */
    ejsSetProperty(ejs, ejs->global, EJSLOT_decimal, (EjsVar*) type);
}


#else
void __dummyDecimal() { }
#endif // BLD_FEATURE_DECIMAL

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsDecimal.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsError.c"
 */
/************************************************************************/

/**
 *  ejsError.c - Error Exception class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castError(Ejs *ejs, EjsError *vp, EjsType *type)
{
    EjsVar      *sp;
    char        *buf;

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_String:
        if (mprAllocSprintf(ejs, &buf, 0, 
                "%s Exception: %s\nStack:\n%s\n", vp->obj.var.type->qname.name, vp->message, vp->stack) < 0) {
            ejsThrowMemoryError(ejs);
        }
        sp = (EjsVar*) ejsCreateString(ejs, buf);
        mprFree(buf);
        return sp;

    default:
        ejsThrowTypeError(ejs, "Unknown type");
        return 0;
    }
}


/*
 *  Get a property.
 */
static EjsVar *getErrorProperty(Ejs *ejs, EjsError *error, int slotNum)
{
    switch (slotNum) {
    case ES_Error_stack:
        return (EjsVar*) ejsCreateString(ejs, error->stack);

    case ES_Error_message:
        return (EjsVar*) ejsCreateString(ejs, error->message);
    }
    return (ejs->objectHelpers->getProperty)(ejs, (EjsVar*) error, slotNum);
}


/*
 *  Error Constructor
 *
 *  public function Error(message: String = null)
 */
static EjsVar *errorConstructor(Ejs *ejs, EjsError *vp, int argc,  EjsVar **argv)
{
    mprFree(vp->message);
    if (argc == 0) {
        vp->message = mprStrdup(vp, "");
    } else {
        vp->message = mprStrdup(vp, ejsGetString(argv[0]));
    }

    mprFree(vp->stack);
    vp->stack = ejsFormatStack(ejs);

    return (EjsVar*) vp;
}


static EjsVar *getCode(Ejs *ejs, EjsError *vp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, vp->code);
}


static EjsVar *setCode(Ejs *ejs, EjsError *vp, int argc,  EjsVar **argv)
{
    vp->code = ejsGetInt(argv[0]);
    return 0;
}




static EjsType *createErrorType(Ejs *ejs, cchar *name, int numClassProp, int numInstanceProp)
{
    EjsType     *type, *baseType;
    EjsName     qname;
    int         flags;

    flags = EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_OBJECT_HELPERS | 
        EJS_ATTR_HAS_CONSTRUCTOR;
    baseType = (ejs->errorType) ? ejs->errorType: ejs->objectType;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, name), baseType, sizeof(EjsError), 
        ES_Error, numClassProp, numInstanceProp, flags);
    type->helpers->castVar = (EjsCastVarHelper) castError;
    type->helpers->getProperty = (EjsGetPropertyHelper) getErrorProperty;

    return type;
}


static void defineType(Ejs *ejs, int slotNum)
{
    EjsType     *type;

    type = ejsGetType(ejs, slotNum);
    ejsBindMethod(ejs, type, type->block.numInherited, (EjsNativeFunction) errorConstructor);
}


void ejsCreateErrorType(Ejs *ejs)
{
    ejs->errorType = createErrorType(ejs, "Error",  ES_Error_NUM_CLASS_PROP, ES_Error_NUM_INSTANCE_PROP);

    createErrorType(ejs, "ArgError", ES_ArgError_NUM_CLASS_PROP, ES_ArgError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "ArithmeticError", ES_ArithmeticError_NUM_CLASS_PROP, ES_ArithmeticError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "AssertError", ES_AssertError_NUM_CLASS_PROP, ES_AssertError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "InstructionError", ES_InstructionError_NUM_CLASS_PROP, ES_InstructionError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "IOError", ES_IOError_NUM_CLASS_PROP, ES_IOError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "InternalError", ES_InternalError_NUM_CLASS_PROP, ES_InternalError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "MemoryError", ES_MemoryError_NUM_CLASS_PROP, ES_MemoryError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "OutOfBoundsError", ES_OutOfBoundsError_NUM_CLASS_PROP, ES_OutOfBoundsError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "ReferenceError", ES_ReferenceError_NUM_CLASS_PROP, ES_ReferenceError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "ResourceError", ES_ResourceError_NUM_CLASS_PROP, ES_ResourceError_NUM_INSTANCE_PROP);
    
#if ES_SecurityError
    createErrorType(ejs, "SecurityError", ES_SecurityError_NUM_CLASS_PROP, ES_SecurityError_NUM_INSTANCE_PROP);
#endif
    createErrorType(ejs, "StateError", ES_StateError_NUM_CLASS_PROP, ES_StateError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "SyntaxError", ES_SyntaxError_NUM_CLASS_PROP, ES_SyntaxError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "TypeError", ES_TypeError_NUM_CLASS_PROP, ES_TypeError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "URIError", ES_URIError_NUM_CLASS_PROP, ES_URIError_NUM_INSTANCE_PROP);
}


void ejsConfigureErrorType(Ejs *ejs)
{
    defineType(ejs, ES_Error);

    ejsBindMethod(ejs, ejs->errorType, ES_Error_code, (EjsNativeFunction) getCode);
    ejsBindMethod(ejs, ejs->errorType, ES_Error_set_code, (EjsNativeFunction) setCode);

    defineType(ejs, ES_ArgError);
    defineType(ejs, ES_ArithmeticError);
    defineType(ejs, ES_AssertError);
    defineType(ejs, ES_InstructionError);
    defineType(ejs, ES_IOError);
    defineType(ejs, ES_InternalError);
    defineType(ejs, ES_MemoryError);
    defineType(ejs, ES_OutOfBoundsError);
    defineType(ejs, ES_ReferenceError);
    defineType(ejs, ES_ResourceError);
#if ES_SecurityError
    defineType(ejs, ES_SecurityError);
#endif
    defineType(ejs, ES_StateError);
    defineType(ejs, ES_SyntaxError);
    defineType(ejs, ES_TypeError);
    defineType(ejs, ES_URIError);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsError.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsFunction.c"
 */
/************************************************************************/

/**
 *  ejsFunction.c - Function class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castFunction(Ejs *ejs, EjsFunction *vp, EjsType *type)
{
    switch (type->id) {
    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "[function Function]");

    case ES_Number:
        return (EjsVar*) ejs->nanValue;

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;
            
    default:
        ejsThrowTypeError(ejs, "Can't cast type \"%s\"", type->qname.name);
        return 0;
    }
    return 0;
}


//  TODO need deep copy. This just does a shallow copy for now.

static EjsFunction *cloneFunctionVar(Ejs *ejs, EjsFunction *src, bool deep)
{
    EjsFunction     *dest;

    dest = (EjsFunction*) ejsCopyBlock(ejs, &src->block, deep);
    if (dest == 0) {
        return 0;
    }
    dest->body.code = src->body.code;
    dest->resultType = src->resultType;
    dest->thisObj = src->thisObj;
    dest->owner = src->owner;
    dest->slotNum = src->slotNum;
    dest->numArgs = src->numArgs;
    dest->numDefault = src->numDefault;
    dest->nextSlot = src->nextSlot;
    /*
     *  TODO OPT - bit fields
     */
    dest->getter = src->getter;
    dest->setter = src->setter;
    dest->staticMethod = src->staticMethod;
    dest->constructor = src->constructor;
    dest->hasReturn = src->hasReturn;
    dest->isInitializer = src->isInitializer;
    dest->literalGetter = src->literalGetter;
    dest->override = src->override;
    dest->rest = src->rest;
    dest->fullScope = src->fullScope;
    dest->lang = src->lang;

    return dest;
}


static void markFunctionVar(Ejs *ejs, EjsVar *parent, EjsFunction *fun)
{
    ejsMarkBlock(ejs, parent, (EjsBlock*) fun);

    if (fun->owner) {
        ejsMarkVar(ejs, parent, fun->owner);
    }
    if (fun->thisObj) {
        ejsMarkVar(ejs, parent, fun->thisObj);
    }
}


static EjsVar *applyFunction(Ejs *ejs, EjsFunction *fun, int argc, EjsVar **argv)
{
    EjsArray        *args;
    EjsVar          *save, *result;
    EjsFrame        *frame;
    
    mprAssert(argc == 2);
    args = (EjsArray*) argv[1];
    mprAssert(ejsIsArray(args));

    frame = ejs->frame;

    //  TODO - need to protect fun from GC while here
    save = fun->thisObj;
    fun->thisObj = 0;

    result =  ejsRunFunction(ejs, fun, argv[0], args->length, args->data);

    mprAssert(frame == ejs->frame);

    fun->thisObj = save;
    return result;
}


static EjsVar *callFunctionMethod(Ejs *ejs, EjsFunction *fun, int argc, EjsVar **argv)
{
    return applyFunction(ejs, fun, argc, argv);
}

/*
 *  Create a script function. This defines the method traits. It does not create a  method slot. ResultType may
 *  be null to indicate untyped. NOTE: untyped functions may return a result at their descretion.
 */

//TODO - numExceptions doesn't belong here. Should be incremented via AddException
//TODO - rather than EjsConst, should be a module reference?

EjsFunction *ejsCreateFunction(Ejs *ejs, const uchar *byteCode, int codeLen, int numArgs, int numExceptions, 
        EjsType *resultType, int attributes, EjsConst *constants, EjsBlock *scopeChain, int lang)
{
    EjsFunction     *fun;
    EjsCode         *code;

    fun = (EjsFunction*) ejsCreateVar(ejs, ejs->functionType, 0);
    if (fun == 0) {
        return 0;
    }

    fun->block.obj.var.isFunction = 1;

    fun->numArgs = numArgs;
    fun->resultType = resultType;
    fun->slotNum = -1;
    fun->nextSlot = -1;
    fun->numArgs = numArgs;
    fun->resultType = resultType;
    fun->block.scopeChain = scopeChain;
    fun->lang = lang;

    /*
     *  When functions are in object literals, we dely setting .getter until the object is actually created.
     *  This enables reading the function without running the getter in the VM.
     */
    if (attributes & EJS_ATTR_LITERAL_GETTER) {
        fun->literalGetter = 1;

    } else if (attributes & EJS_ATTR_GETTER) {
        fun->getter = 1;
    }
    if (attributes & EJS_ATTR_SETTER) {
        fun->setter = 1;
    }
    if (attributes & EJS_ATTR_CONSTRUCTOR) {
        fun->constructor = 1;
    }
    if (attributes & EJS_ATTR_REST) {
        fun->rest = 1;
    }
    if (attributes & EJS_ATTR_STATIC) {
        fun->staticMethod = 1;
    }
    if (attributes & EJS_ATTR_OVERRIDE) {
        fun->override = 1;
    }
    if (attributes & EJS_ATTR_NATIVE) {
        fun->block.obj.var.nativeProc = 1;
    }
    if (attributes & EJS_ATTR_FULL_SCOPE) {
        fun->fullScope = 1;
    }
    if (attributes & EJS_ATTR_HAS_RETURN) {
        fun->hasReturn = 1;
    }

    code = &fun->body.code;
    code->codeLen = codeLen;
    code->byteCode = (uchar*) byteCode;
    code->numHandlers = numExceptions;
    code->constants = constants;
    code->finallyIndex = -1;

    return fun;
}


void ejsSetNextFunction(EjsFunction *fun, int nextSlot)
{
    fun->nextSlot = nextSlot;
}


void ejsSetFunctionLocation(EjsFunction *fun, EjsVar *obj, int slotNum)
{
    mprAssert(fun);
    mprAssert(obj);

    fun->owner = obj;
    fun->slotNum = slotNum;
}


EjsEx *ejsAddException(EjsFunction *fun, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart,
        uint handlerEnd, int flags, int preferredIndex)
{
    EjsEx           *exception;
    EjsCode         *code;
    int             size;

    mprAssert(fun);

    code = &fun->body.code;

    exception = mprAllocObjZeroed(fun, EjsEx);
    if (exception == 0) {
        mprAssert(0);
        return 0;
    }

    exception->flags = flags;
    exception->tryStart = tryStart;
    exception->tryEnd = tryEnd;
    exception->catchType = catchType;
    exception->handlerStart = handlerStart;
    exception->handlerEnd = handlerEnd;

    if (preferredIndex < 0) {
        preferredIndex = code->numHandlers++;
    }

    if (preferredIndex >= code->sizeHandlers) {
        size = code->sizeHandlers + EJS_EX_INC;
        code->handlers = (EjsEx**) mprRealloc(fun, code->handlers, size * sizeof(EjsEx));
        if (code->handlers == 0) {
            mprAssert(0);
            return 0;
        }
        memset(&code->handlers[code->sizeHandlers], 0, EJS_EX_INC * sizeof(EjsEx)); 
        code->sizeHandlers = size;
    }

    if (flags & EJS_EX_FINALLY) {
        code->finallyIndex = preferredIndex;
    }

    code->handlers[preferredIndex] = exception;

    return exception;
}


void ejsOffsetExceptions(EjsFunction *fun, int offset)
{
    EjsEx           *ex;
    int             i;

    mprAssert(fun);

    for (i = 0; i < fun->body.code.numHandlers; i++) {
        ex = fun->body.code.handlers[i];
        ex->tryStart += offset;
        ex->tryEnd += offset;
        ex->handlerStart += offset;
        ex->handlerEnd += offset;
    }
}


/*
 *  Set the byte code for a script function
 */
int ejsSetFunctionCode(EjsFunction *fun, uchar *byteCode, int len)
{
    mprAssert(fun);
    mprAssert(byteCode);
    mprAssert(len >= 0);

    byteCode = (uchar*) mprMemdup(fun, byteCode, len);
    if (byteCode == 0) {
        return EJS_ERR;
    }

    fun->body.code.codeLen = len;
    mprFree(fun->body.code.byteCode);
    fun->body.code.byteCode = (uchar*) byteCode;

    return 0;
}


EjsFunction *ejsCopyFunction(Ejs *ejs, EjsFunction *src)
{
    return cloneFunctionVar(ejs, src, 0);
}


void ejsCreateFunctionType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Function"), ejs->objectType, sizeof(EjsFunction), 
        ES_Function, ES_Function_NUM_CLASS_PROP, ES_Function_NUM_INSTANCE_PROP, 
        EJS_ATTR_OBJECT | EJS_ATTR_NATIVE | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_BLOCK_HELPERS);
    ejs->functionType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar   = (EjsCastVarHelper) castFunction;
    type->helpers->cloneVar  = (EjsCloneVarHelper) cloneFunctionVar;
    type->helpers->markVar   = (EjsMarkVarHelper) markFunctionVar;

#if UNUSED
    type->skipScope = 1;
#endif
}


void ejsConfigureFunctionType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->functionType;
    ejsBindMethod(ejs, type, ES_Function_apply, (EjsNativeFunction) applyFunction);
    ejsBindMethod(ejs, type, ES_Function_call, (EjsNativeFunction) callFunctionMethod);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsFunction.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsGlobal.c"
 */
/************************************************************************/

/**
 *  ejsGlobal.c - Global functions and variables
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static EjsVar *parseLiteral(Ejs *ejs, char **next);

/**
 *  Assert a condition is true.
 *
 *  public static function assert(condition: Boolean): Boolean
 */
static EjsVar *assertMethod(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    EjsBoolean      *b;

    mprAssert(argc == 1);

    if (! ejsIsBoolean(argv[0])) {
        b = (EjsBoolean*) ejsCastVar(ejs, argv[0], ejs->booleanType);
    } else {
        b = (EjsBoolean*) argv[0];
    }
    mprAssert(b);

    if (b == 0 || !b->value) {
#if BLD_DEBUG
        if (ejs->frame->currentLine) {
            mprLog(ejs, 0, "Assertion error: %s", ejs->frame->currentLine);
            ejsThrowAssertError(ejs, "Assertion error: %s", ejs->frame->currentLine);
        } else
#endif
            ejsThrowAssertError(ejs, "Assertion error");
        return 0;
    }
    return vp;
}


#if MOVE_TO_DEBUG_CLASS || 1
/**
 *  Trap to the debugger
 *
 *  public static function breakpoint(): Void
 */
static EjsVar *breakpoint(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    mprBreakpoint();
    return 0;
}
#endif


/**
 *  Clone the base class. Used by Record.es
 *  TODO - where to move this?
 *
 *  public static function cloneBase(klass: Type): Void
 */
static EjsVar *cloneBase(Ejs *ejs, EjsVar *ignored, int argc, EjsVar **argv)
{
    EjsType     *type;
    
    mprAssert(argc == 1);
    
    type = (EjsType*) argv[0];
    type->baseType = (EjsType*) ejsCloneVar(ejs, (EjsVar*) type->baseType, 0);
    ejsSetReference(ejs, (EjsVar*) type, (EjsVar*) type->baseType);
    return 0;
}



//  TODO - merge with parse() to handle all types
//  TODO - this parser is not robust enough. Need lookahead - rewrite.
EjsVar *ejsDeserialize(Ejs *ejs, EjsVar *value)
{
    EjsVar      *obj;
    char        *str, *next;

    if (!ejsIsString(value)) {
        return 0;
    }
    str = ejsGetString(value);
    if (str == 0) {
        return 0;
    } else if (*str == '\0') {
        return (EjsVar*) ejs->emptyStringValue;
    }

    next = str;
    if ((obj = parseLiteral(ejs, &next)) == 0) {
        ejsThrowSyntaxError(ejs, "Can't parse object literal");
        return 0;
    }
    return obj;
}


/*
 *  Convert a string into an object.
 *
 *  intrinsic native static function deserialize(obj: String): Object
 */
EjsVar *deserialize(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsString(argv[0]));
    return ejsDeserialize(ejs, argv[0]);
}


typedef enum Token {
    TOK_EOF,
    TOK_LBRACE,
    TOK_LBRACKET,
    TOK_RBRACE,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_COMMA,
    TOK_ID,
    TOK_QID,            /* QUOTED */
} Token;

//  TODO - get more unique name for all-in-one

Token getToken(char *token, char **next)
{
    char    *cp, *start, *firstQuote, *src, *dest;
    int     quote, tid;

    for (cp = *next; isspace((int) *cp); cp++) {
        ;
    }

    *next = cp + 1;
    firstQuote = 0;

    if (*cp == '\0') {
        tid = TOK_EOF;

    } else  if (*cp == '{') {
        tid = TOK_LBRACE;

    } else if (*cp == '[') {
        tid = TOK_LBRACKET;

    } else if (*cp == '}' || *cp == ']') {
        tid = *cp == '}' ? TOK_RBRACE: TOK_RBRACKET;
        while (*++cp && isspace((int) *cp)) {
            ;
        }
        if (*cp == ',' || *cp == ':') {
            cp++;
        }
        *next = cp;

    } else if (*cp == ':') {
        tid = TOK_COLON;

    } else {
        quote = 0;
        start = cp;
        for (start = cp; *cp; cp++) {
            if (*cp == '\\') {
                cp++;
            } else if (*cp == '\'' || *cp == '\"') {
                if (quote) {
                    if (*cp == quote && cp > firstQuote && cp[-1] != '\\') {
                        *cp = '\0';
                        quote = 0;
                    }
                } else {
                    firstQuote = cp;
                    start = cp + 1;
                    quote = *cp;
                }
                continue;
            }
            if (!quote && (*cp == ',' || *cp == ':')) {
                break;
            }
            if (!quote && (*cp == ']' || *cp == '}')) {
                break;
            }
        }
        if (quote && *start == quote) {
            /* No matching end */
            start--;
        }
        strncpy(token, start, min(MPR_MAX_STRING, cp - start));
        token[cp - start] = '\0';
        if (*cp == ',' || *cp == ':') {
            cp++;
        }
        *next = cp;

        for (dest = src = token; *src; ) {
            if (*src == '\\') {
                src++;
            }
            *dest++ = *src++;
        }
        *dest = '\0';

        tid = (firstQuote) ? TOK_QID : TOK_ID;
    }
    return tid;
}


/*
 *  Parse an object literal string pointed to by *next into the given object. Update *next to point
 *  to the next input token in the object literal. Supports nested object literals.
 *  TODO - should be rewritten to handle unbounded key and value tokens
 */
static EjsVar *parseLiteral(Ejs *ejs, char **next)
{
    EjsName     qname;
    EjsVar      *obj, *vp;
    char        *kp, *prior;
    char        token[MPR_MAX_STRING], key[MPR_MAX_STRING], value[MPR_MAX_STRING];
    int         tid, isArray;

    isArray = 0;

    tid = getToken(token, next);
    if (tid == TOK_EOF) {
        return 0;
    }

    if (tid == TOK_LBRACKET) {
        isArray = 1;
        obj = (EjsVar*) ejsCreateArray(ejs, 0);

    } else if (tid == TOK_LBRACE) {
        obj = (EjsVar*) ejsCreateObject(ejs, ejs->objectType, 0);

    } else {
        return ejsParseVar(ejs, token, -1);
    }
    if (obj == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    while (1) {
        prior = *next;
        tid = getToken(key, next);
        if (tid == TOK_EOF) {
            return obj;
        } else if (tid == TOK_RBRACE || tid == TOK_RBRACKET) {
            break;
        }

        if (tid == TOK_LBRACKET) {
            *next = prior;
            vp = parseLiteral(ejs, next);

        } else if (tid == TOK_LBRACE) {
            *next = prior;
            vp = parseLiteral(ejs, next);

        } else if (isArray) {
            vp = ejsParseVar(ejs, key, -1);

        } else {
            //  TODO - really need a peek
            prior = *next;
            tid = getToken(value, next);
            if (tid == TOK_LBRACE || tid == TOK_LBRACKET) {
                *next = prior;
                vp = parseLiteral(ejs, next);

            } else if (tid != TOK_ID && tid != TOK_QID) {
                return 0;

            } else {
                if (tid == TOK_QID) {
                    /* Quoted */
                    vp = (EjsVar*) ejsCreateString(ejs, value);
                } else {
                    if (strcmp(value, "null") == 0) {
                        vp = ejs->nullValue;
                    } else if (strcmp(value, "undefined") == 0) {
                        vp = ejs->undefinedValue;
                    } else {
                        vp = ejsParseVar(ejs, value, -1);
                    }
                }
            }
        }
        if (vp == 0) {
            return 0;
        }

        if (isArray) {
            if (ejsSetProperty(ejs, obj, -1, vp) < 0) {
                return 0;
            }

        } else {
            //  TODO - bug. Could leak if this object is put back on the object type pool and reused in this manner.
            kp = mprStrdup(obj, key);
            ejsName(&qname, EJS_PUBLIC_NAMESPACE, kp);
            if (ejsSetPropertyByName(ejs, obj, &qname, vp) < 0) {
                return 0;
            }
        }

    }
    return obj;
}


/**
 *  Format the stack
 *
 *  public static function formatStack(): String
 */
static EjsVar *formatStackMethod(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, ejsFormatStack(ejs));
}


#if ES_hashcode
/*
 *  Get the hash code for the object.
 *
 *  intrinsic function hashcode(o: Object): Number
 */
static EjsVar *hashcode(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1);
    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) PTOL(argv[0]));
}
#endif


/**
 *  Load a script or module. Name should have an extension. Name will be located according to the EJSPATH search strategy.
 *
 *  public static function load(name: String): void
 */
static EjsVar *load(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    cchar       *name, *cp;
    char        *path;

    name = ejsGetString(argv[0]);

    if ((cp = strrchr(name, '.')) != NULL && strcmp(cp, ".es") == 0) {
        ejsThrowIOError(ejs, "load: Compiling is not enabled for %s", name);

    } else {
        if (ejsSearch(ejs, &path, name) < 0) {
            ejsThrowIOError(ejs, "Can't find %s to load", name);
        } else {
            ejsLoadModule(ejs, path, NULL, NULL, 0);
        }
    }
    return 0;
}


/**
 *  Parse the input and convert to a primitive type
 *
 *  public static function parse(input: String, preferredType: Type = null): void
 */
static EjsVar *parse(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    cchar       *input;
    int         preferred;

    input = ejsGetString(argv[0]);

    if (argc == 2 && !ejsIsType(argv[1])) {
        ejsThrowArgError(ejs, "Argument is not a type");
        return 0;
    }
    preferred = (argc == 2) ? ((EjsType*) argv[1])->id : -1;

    return ejsParseVar(ejs, input, preferred);
}


/**
 *  Print the arguments to the standard error with a new line.
 *
 *  public static function eprint(...args): void
 */
static EjsVar *eprintMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString   *s;
    EjsVar      *args, *vp;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp == 0) {
            s = 0;
        } else {
            s = (EjsString*) ejsToString(ejs, vp);
            if (ejs->exception) {
                return 0;
            }
        }
        if (s) {
            mprErrorPrintf(ejs, "%s", s->value);
        }
    }
    mprErrorPrintf(ejs, "\n");
    return 0;
}


/**
 *  Print the arguments to the standard output with a new line.
 *
 *  public static function print(...args): void
 */
static EjsVar *printMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString   *s;
    EjsVar      *args, *vp;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp == 0) {
            s = 0;
        } else {
            s = (EjsString*) ejsToString(ejs, vp);
            if (ejs->exception) {
                return 0;
            }
        }
        if (s) {
            mprPrintf(ejs, "%s", s->value);
        }
    }
    mprPrintf(ejs, "\n");
    return 0;
}


#if ES_printv && BLD_DEBUG
/**
 *  Print the named variables for debugging.
 *
 *  public static function printv(...args): void
 */
static EjsVar *printv(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString   *s;
    EjsVar      *args, *vp;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp == 0) {
            continue;
        }

        s = (EjsString*) ejsToString(ejs, vp);

        if (ejs->exception) {
            return 0;
        }

        mprAssert(s && ejsIsString(s));
        mprPrintf(ejs, "%s = %s\n", vp->debugName, s->value);
    }
    mprPrintf(ejs, "\n");
    return 0;
}
#endif


EjsVar *ejsSerialize(Ejs *ejs, EjsVar *vp, int maxDepth, bool showAll, bool showBase)
{
    EjsString       *result;
    EjsVar          *pp;
    EjsObject       *obj;
    EjsString       *sv;
    EjsBlock        *block;
    MprBuf          *buf;
    EjsName         qname;
    char            key[16], *cp;
    int             i, slotNum, numInherited, flags, isArray, count;

    if (maxDepth == 0) {
        maxDepth = MAXINT;
    }
    flags = 0;
    if (showAll) {
        flags |= EJS_FLAGS_ENUM_ALL;
    }
    if (showBase) {
        flags |= EJS_FLAGS_ENUM_INHERITED;
    }

    buf = mprCreateBuf(vp, 0, 0);
    if (buf == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    /*
     *  TODO - this whole strategy is flawed. Need a serialize helper per object
     */
    count = ejsGetPropertyCount(ejs, vp);

    if (count == 0 || ejsIsXML(vp)) {

        if (ejsIsFunction(vp)) {
            mprPutStringToBuf(buf, "[function]");

        } else if (ejsIsString(vp) || ejsIsXML(vp)) {
            mprPutCharToBuf(buf, '\"');
            sv = (EjsString*) ejsToString(ejs, vp);
            if (strchr(sv->value, '"')) {
                for (cp = sv->value; cp < &sv->value[sv->length]; cp++) {
                    if (*cp == '"') {
                        mprPutCharToBuf(buf, '\\');
                        mprPutCharToBuf(buf, *cp);
                    } else {
                        mprPutCharToBuf(buf, *cp);
                    }
                }
            } else {
                mprPutStringToBuf(buf, sv->value);
            }
            mprPutCharToBuf(buf, '\"');

        } else if (ejsIsObject(vp)) {
            mprPutStringToBuf(buf, "{}");

        } else {
            sv = (EjsString*) ejsToString(ejs, vp);
            mprPutStringToBuf(buf, sv->value);
        }

    } else {

        if (vp->visited) {
            return (EjsVar*) ejsCreateString(ejs, "this");
        }

        isArray = ejsIsArray(vp);
        mprPutStringToBuf(buf, isArray ? "[\n" : "{\n");

        vp->visited = 1;
        if (++ejs->serializeDepth <= maxDepth /* && ejsIsObject(vp) */) {

            obj = (EjsObject*) vp;

            for (slotNum = 0; slotNum < count; slotNum++) {
                if (ejsIsBlock(vp)) {
                    block = (EjsBlock*) vp;
                    numInherited = ejsGetNumInheritedTraits((EjsBlock*) obj);
                    if (slotNum < numInherited && !(flags & EJS_FLAGS_ENUM_INHERITED)) {
                        continue;
                    }
                }
                pp = ejsGetProperty(ejs, vp, slotNum);
                
                if (pp == 0 || (pp->hidden && !(flags & EJS_FLAGS_ENUM_ALL))) {
                    if (ejs->exception) {
                        return 0;
                    }
                    continue;
                }
                if (isArray) {
                    mprItoa(key, sizeof(key), slotNum, 10);
                    qname.name = key;
                    qname.space = "";
                } else {
                    qname = ejsGetPropertyName(ejs, vp, slotNum);
                }
                for (i = 0; i < ejs->serializeDepth; i++) {
                    mprPutStringToBuf(buf, "  ");
                }
                if (!isArray) {
                    if (flags & EJS_FLAGS_ENUM_ALL) {
                        mprPutFmtToBuf(buf, "%s_%s: ", qname.space, qname.name);
                    } else {
                        mprPutFmtToBuf(buf, "%s: ", qname.name);
                    }
                }

                sv = (EjsString*) ejsSerialize(ejs, pp, maxDepth, showAll, showBase);
                if (sv == 0 || !ejsIsString(sv)) {
                    if (!ejs->exception) {
                        ejsThrowTypeError(ejs, "Cant serialize property %s", qname.name);
                    }
                    return 0;

                } else {
                    mprPutStringToBuf(buf, sv->value);
                }

                mprPutStringToBuf(buf, ",\n");
            }
        }
        vp->visited = 0;

        ejs->serializeDepth--;
        for (i = 0; i < ejs->serializeDepth; i++) {
            mprPutStringToBuf(buf, "  ");
        }
        mprPutCharToBuf(buf, isArray ? ']' : '}');
    }

    mprAddNullToBuf(buf);
    result = ejsCreateString(ejs, mprGetBufStart(buf));
    mprFree(buf);

    return (EjsVar*) result;
}


/*
 *  Convert the object to a source code string.
 *
 *  intrinsic function serialize(obj: Object, maxDepth: Number = 0, showAll: Boolean = false, 
 *      showBase: Boolean = false): String
 */
EjsVar *serialize(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsVar          *vp;
    int             flags, maxDepth;
    bool            showBase, showAll;

    flags = 0;
    maxDepth = MAXINT;

    vp = argv[0];

    if (argc >= 2) {
        maxDepth = ejsGetInt(argv[1]);
    }

    showAll = (argc >= 3 && argv[2] == (EjsVar*) ejs->trueValue);
    showBase = (argc == 4 && argv[3] == (EjsVar*) ejs->trueValue);
    return ejsSerialize(ejs, argv[0], maxDepth, showAll, showBase);
}


static EjsNamespace *addNamespace(Ejs *ejs, EjsBlock *block, cchar *space)
{
    EjsNamespace    *ns;

    ns = ejsDefineReservedNamespace(ejs, block, 0, space);
    mprAddHash(ejs->standardSpaces, space, ns);

    return ns;
}


void ejsCreateGlobalBlock(Ejs *ejs)
{
    EjsBlock    *block;

    /*
     *  Pre-create extra global slots
     */
    ejs->globalBlock = ejsCreateBlock(ejs, EJS_GLOBAL, max(ES_global_NUM_CLASS_PROP, 256));
    ejs->global = (EjsVar*) ejs->globalBlock;
    
    if ((ejs->flags & EJS_FLAG_COMPILER) && (ejs->flags & EJS_FLAG_EMPTY)) {
        ejs->globalBlock->obj.numProp = 0;
    } else {
        ejs->globalBlock->obj.numProp = ES_global_NUM_CLASS_PROP;
    }
    
    block = (EjsBlock*) ejs->global;
    
    /*
     *  Create the standard namespaces. Order matters here. This is the (reverse) order of lookup.
     */
    ejs->emptySpace =       addNamespace(ejs, block, EJS_EMPTY_NAMESPACE);
    ejs->configSpace =      addNamespace(ejs, block, EJS_CONFIG_NAMESPACE);
    ejs->iteratorSpace =    addNamespace(ejs, block, EJS_ITERATOR_NAMESPACE);
    ejs->intrinsicSpace =   addNamespace(ejs, block, EJS_INTRINSIC_NAMESPACE);
    ejs->eventsSpace =      addNamespace(ejs, block, EJS_EVENTS_NAMESPACE);
    ejs->ioSpace =          addNamespace(ejs, block, EJS_IO_NAMESPACE);
    ejs->sysSpace =         addNamespace(ejs, block, EJS_SYS_NAMESPACE);
    ejs->publicSpace =      addNamespace(ejs, block, EJS_PUBLIC_NAMESPACE);
}


void ejsConfigureGlobalBlock(Ejs *ejs)
{
    EjsBlock    *block;

    block = ejs->globalBlock;
    mprAssert(block);

#if ES_assert
    ejsBindFunction(ejs, block, ES_assert, assertMethod);
#endif
#if ES_breakpoint
    ejsBindFunction(ejs, block, ES_breakpoint, breakpoint);
#endif
#if ES_cloneBase
    ejsBindFunction(ejs, block, ES_cloneBase, (EjsNativeFunction) cloneBase);
#endif
#if ES_deserialize
    ejsBindFunction(ejs, block, ES_deserialize, deserialize);
#endif
#if ES_exit
    ejsBindFunction(ejs, block, ES_exit, exitMethod);
#endif
#if ES_formatStack
    ejsBindFunction(ejs, block, ES_formatStack, formatStackMethod);
#endif
#if ES_hashcode
    ejsBindFunction(ejs, block, ES_hashcode, hashcode);
#endif
#if ES_load
    ejsBindFunction(ejs, block, ES_load, load);
#endif
#if ES_parse
    ejsBindFunction(ejs, block, ES_parse, parse);
#endif
#if ES_eprint
    ejsBindFunction(ejs, block, ES_eprint, eprintMethod);
#endif
#if ES_print
    ejsBindFunction(ejs, block, ES_print, printMethod);
#endif
#if ES_printv && BLD_DEBUG
    ejsBindFunction(ejs, block, ES_printv, printv);
#endif
#if ES_serialize
    ejsBindFunction(ejs, block, ES_serialize, serialize);
#endif
    /*
     *  Update the global reference
     */
    ejsSetProperty(ejs, ejs->global, ES_global, ejs->global);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsGlobal.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsIterator.c"
 */
/************************************************************************/

/**
 *  ejsIterator.c - Iterator class
 *
 *  This provides a high performance iterator construction for native classes.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static void markIteratorVar(Ejs *ejs, EjsVar *parent, EjsIterator *ip)
{
    if (ip->target) {
        ejsMarkVar(ejs, (EjsVar*) ip, ip->target);
    }
}


/*
 *  Call the supplied next() function to return the next enumerable item
 */
static EjsVar *nextIterator(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    if (ip->nativeNext) {
        return (ip->nativeNext)(ejs, (EjsVar*) ip, argc, argv);
    } else {
        ejsThrowStopIteration(ejs);
        return 0;
    }
#if UNUSED && TODO
    if (ip->nativeNext) {
        return (ip->nativeNext)(ejs, (EjsVar*) ip, argc, argv);
    }
    return ejsRunFunction(ejs, ip->next, ip->target, argc, argv);
#endif
}


/*
 *  Throw the StopIteration object
 */
EjsVar *ejsThrowStopIteration(Ejs *ejs)
{
    return ejsThrowException(ejs, (EjsVar*) ejs->stopIterationType);
}


#if UNUSED
/*
 *  Constructor to create an iterator using a scripted next().
 *
 *  public function Iterator(obj, f,    TODO ECMA deep, ...namespaces)
 */
static EjsVar *iteratorConstructor(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    if (argc != 2 || !ejsIsFunction(argv[1])) {
        ejsThrowArgError(ejs, "usage: Iterator(obj, function)");
        return 0;
    }
    ip->target = argv[0];
    ip->next = (EjsFunction*) argv[1];
    mprAssert(ip->nativeNext == 0);

    return (EjsVar*) ip;
}
#endif


/*
 *  Create an iterator.
 */
EjsIterator *ejsCreateIterator(Ejs *ejs, EjsVar *obj, EjsNativeFunction nativeNext, bool deep, EjsArray *namespaces)
{
    EjsIterator     *ip;

    ip = (EjsIterator*) ejsCreateVar(ejs, ejs->iteratorType, 0);
    if (ip) {
        ip->nativeNext = nativeNext;
        ip->target = obj;
        ip->deep = deep;
        ip->namespaces = namespaces;
        ejsSetDebugName(ip, "iterator");
    }
    return ip;
}


/*
 *  Create the Iterator and StopIteration types
 */
void ejsCreateIteratorType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_ITERATOR_NAMESPACE, "Iterator"), ejs->objectType, sizeof(EjsIterator),
        ES_Iterator, ES_Iterator_NUM_CLASS_PROP, ES_Iterator_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->iteratorType = type;

    type->helpers->markVar  = (EjsMarkVarHelper) markIteratorVar;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_ITERATOR_NAMESPACE, "StopIteration"), ejs->objectType, sizeof(EjsVar), 
        ES_StopIteration, ES_StopIteration_NUM_CLASS_PROP,  ES_StopIteration_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->stopIterationType = type;
}


void ejsConfigureIteratorType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->iteratorType;

    /*
     *  Define the "next" method
     */
    ejsBindMethod(ejs, ejs->iteratorType, ES_Iterator_next, (EjsNativeFunction) nextIterator);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsIterator.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsNamespace.c"
 */
/************************************************************************/

/**
 *  ejsNamespace.c - Ejscript Namespace class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Cast the operand to the specified type
 */

static EjsVar *castNamespace(Ejs *ejs, EjsNamespace *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "[object Namespace]");

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


static EjsVar *invokeNamespaceOperator(Ejs *ejs, EjsNamespace *lhs, int opCode, EjsNamespace *rhs)
{
    bool        boolResult;

    switch (opCode) {
    case EJS_OP_COMPARE_EQ:
        boolResult = (strcmp(lhs->name, rhs->name) == 0 && strcmp(lhs->uri, rhs->uri) == 0);
        break;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        boolResult = lhs == rhs;
        break;

    case EJS_OP_COMPARE_NE:
        boolResult = ! (strcmp(lhs->name, rhs->name) == 0 && strcmp(lhs->uri, rhs->uri) == 0);
        break;

    case EJS_OP_COMPARE_STRICTLY_NE:
        boolResult = !(lhs == rhs);
        break;

    default:
        ejsThrowTypeError(ejs, "Operation is not valid on this type");
        return 0;
    }
    return (EjsVar*) ejsCreateBoolean(ejs, boolResult);
}


/*
 *  Define a reserved namespace in a block.
 */
EjsNamespace *ejsDefineReservedNamespace(Ejs *ejs, EjsBlock *block, EjsName *typeName, cchar *spaceName)
{
    EjsNamespace    *namespace;

    namespace = ejsCreateReservedNamespace(ejs, typeName, spaceName);
    if (namespace) {
        if (ejsAddNamespaceToBlock(ejs, block, namespace) < 0) {
            return 0;
        }
    }
    return namespace;
}


/*
 *  Format a reserved namespace to create a unique namespace URI. "internal, public, private, protected"
 *
 *  Namespaces are formatted as strings using the following format, where type is optional. Types may be qualified.
 *      [type,space]
 *
 *  Example:
 *      [debug::Shape,public] where Shape was declared as "debug class Shape"
 */
char *ejsFormatReservedNamespace(MprCtx ctx, EjsName *typeName, cchar *spaceName)
{
    cchar   *typeNameSpace;
    char    *namespace, *sp;
    int     len, typeLen, spaceLen, l;

    len = typeLen = spaceLen = 0;
    typeNameSpace = 0;

    if (typeName) {
        if (typeName->name == 0) {
            typeName = 0;
        }
        typeNameSpace = typeName->space ? typeName->space : EJS_PUBLIC_NAMESPACE;
    }

    if (typeName && typeName->name) {
        //  Join the qualified typeName to be "space::name"
        mprAssert(typeName->name);
        typeLen = (int) strlen(typeNameSpace);
        typeLen += 2 + (int) strlen(typeName->name);          //  Allow for the "::" between space::name
        len += typeLen;
    }
    spaceLen = (int) strlen(spaceName);

    /*
     *  Add 4 for [,,]
     *  Add 2 for the trailing "::" and one for the null
     */
    len += 4 + spaceLen + 2 + 1;

    namespace = mprAlloc(ctx, len);
    if (namespace == 0) {
        return 0;
    }

    sp = namespace;
    *sp++ = '[';

    if (typeName) {
        if (strcmp(typeNameSpace, EJS_PUBLIC_NAMESPACE) != 0) {
            l = (int) strlen(typeNameSpace);
            strcpy(sp, typeNameSpace);
            sp += l;
            *sp++ = ':';
            *sp++ = ':';
        }
        l = (int) strlen(typeName->name);
        strcpy(sp, typeName->name);
        sp += l;
    }

    *sp++ = ',';
    strcpy(sp, spaceName);
    sp += spaceLen;

    *sp++ = ']';
    *sp = '\0';

    mprAssert(sp <= &namespace[len]);

    return namespace;
}


/*
 *  Create a namespace with the given URI as its definition qualifying value.
 */
EjsNamespace *ejsCreateNamespace(Ejs *ejs, cchar *name, cchar *uri)
{
    EjsNamespace    *np;

    if (uri == 0) {
        uri = name;
    } else if (name == 0) {
        name = uri;
    }

    np = (EjsNamespace*) ejsCreateVar(ejs, ejs->namespaceType, 0);
    if (np) {
        np->name = (char*) name;
        np->uri = (char*) uri;
    }
    ejsSetDebugName(np, np->uri);
    return np;
}


/*
 *  Create a reserved namespace. Format the package, type and space names to create a unique namespace URI.
 *  packageName, typeName and uri are optional.
 */
EjsNamespace *ejsCreateReservedNamespace(Ejs *ejs, EjsName *typeName, cchar *spaceName)
{
    EjsNamespace    *namespace;
    char            *formattedName;

    mprAssert(spaceName);

    if (typeName) {
        formattedName = (char*) ejsFormatReservedNamespace(ejs, typeName, spaceName);
    } else {
        formattedName = (char*) spaceName;
    }

    namespace = ejsCreateNamespace(ejs, formattedName, formattedName);

    return namespace;
}


void ejsCreateNamespaceType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Namespace"), ejs->objectType, sizeof(EjsNamespace),
        ES_Namespace, ES_Namespace_NUM_CLASS_PROP, ES_Namespace_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->namespaceType = type;
    
    /*
     *  Define the helper functions.
     */
    //  TODO - need to provide lookupProperty to access name and uri
    type->helpers->castVar = (EjsCastVarHelper) castNamespace;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeNamespaceOperator;
}


void ejsConfigureNamespaceType(Ejs *ejs)
{
    ejsSetProperty(ejs, ejs->global, ES_intrinsic, (EjsVar*) ejs->intrinsicSpace);
    ejsSetProperty(ejs, ejs->global, ES_iterator, (EjsVar*) ejs->iteratorSpace);
    ejsSetProperty(ejs, ejs->global, ES_public, (EjsVar*) ejs->publicSpace);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsNamespace.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsNull.c"
 */
/************************************************************************/

/**
 *  ejsNull.c - Ejscript Null class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Cast the null operand to a primitive type
 */

static EjsVar *castNull(Ejs *ejs, EjsVar *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->falseValue;

    case ES_Number:
        return (EjsVar*) ejs->zeroValue;

    case ES_Object:
    default:
        /*
         *  Cast null to anything else results in a null
         */
        return vp;

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "null");
    }
}


static EjsVar *coerceNullOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {

    case EJS_OP_ADD:
        if (!ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        /* Fall through */

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    /*
     *  Comparision
     */
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);
        } else if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->falseValue;
        }
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


static EjsVar *invokeNullOperator(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceNullOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match. Both left and right types are both "null"
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->oneValue;

    /*
     *  Binary operators. Reinvoke with left = zero
     */
    case EJS_OP_ADD: case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }
}


/*
 *  iterator native function get(): Iterator
 */
static EjsVar *getNullIterator(Ejs *ejs, EjsVar *np, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, np, NULL, 0, NULL);
}


/*
 *  We dont actually allocate any nulls. We just reuse the singleton instance.
 */

EjsNull *ejsCreateNull(Ejs *ejs)
{
    return (EjsNull*) ejs->nullValue;
}


void ejsCreateNullType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Null"), ejs->objectType, sizeof(EjsNull),
        ES_Null, ES_Null_NUM_CLASS_PROP, ES_Null_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->nullType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castNull;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeNullOperator;

    ejs->nullValue = ejsCreateVar(ejs, type, 0);
    ejsSetDebugName(ejs->nullValue, "null");
    
    if (!(ejs->flags & EJS_FLAG_EMPTY)) {
        ejsSetProperty(ejs, ejs->global, ES_null, ejs->nullValue);
    }
}


void ejsConfigureNullType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->nullType;

    ejsSetProperty(ejs, ejs->global, ES_null, ejs->nullValue);

    ejsBindMethod(ejs, type, ES_Object_get, getNullIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getNullIterator);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsNull.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsNumber.c"
 */
/************************************************************************/

/**
 *  ejsNumber.c - Number type class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  TODO - move to ejsNumber.h. But would have to rename fixed() to ejsFixed()
 */
#if BLD_FEATURE_FLOATING_POINT
#define fixed(n) ((int64) (floor(n)))
#else
#define fixed(n) (n)
#endif

#if BLD_WIN_LIKE
static double rint(double num)
{
    double low = floor(num);
    double high = ceil(num);
    return ((high - num) >= (num - low)) ? low : high;
}
#endif
/*
 *  Cast the operand to the specified type
 */

static EjsVar *castNumber(Ejs *ejs, EjsNumber *vp, EjsType *type)
{
    char    numBuf[512];

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ((vp->value) ? ejs->trueValue : ejs->falseValue);

    case ES_String:
#if BLD_FEATURE_FLOATING_POINT
        /* TODO - refactor */
        if (rint(vp->value) == vp->value && ((-1 - MAXINT) <= vp->value && vp->value <= MAXINT)) {
            mprSprintf(numBuf, sizeof(numBuf), "%.0f", vp->value);
        } else {
            mprSprintf(numBuf, sizeof(numBuf), "%g", vp->value);
        }
        return (EjsVar*) ejsCreateString(ejs, numBuf);
#elif MPR_64_BIT
        mprSprintf(numBuf, sizeof(numBuf), "%Ld", vp->value);
        return (EjsVar*) ejsCreateString(ejs, numBuf);
#else
        mprItoa(numBuf, sizeof(numBuf), (int) vp->value, 10);
        return (EjsVar*) ejsCreateString(ejs, numBuf);
#endif

    case ES_Number:
        return (EjsVar*) vp;
            
    default:
        return (EjsVar*) ejs->zeroValue;
    }
}


static EjsVar *coerceNumberOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->nanValue;
        } else if (ejsIsNull(rhs)) {
            return (EjsVar*) lhs;
        } else if (ejsIsBoolean(rhs) || ejsIsDate(rhs)) {
            return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));
        } else {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) (((EjsNumber*) lhs)->value ? ejs->trueValue : ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsNumber*) lhs)->value ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


static EjsVar *invokeNumberOperator(Ejs *ejs, EjsNumber *lhs, int opcode, EjsNumber *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceNumberOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match, both numbers
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ((lhs->value == rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ((lhs->value != rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ((lhs->value < rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ((lhs->value <= rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ((lhs->value > rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ((lhs->value >= rhs->value) ? ejs->trueValue: ejs->falseValue);

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_NEG:
        return (EjsVar*) ejsCreateNumber(ejs, -lhs->value);

    case EJS_OP_LOGICAL_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, !fixed(lhs->value));

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (~fixed(lhs->value)));


    /*
     *  Binary operations
     */
    case EJS_OP_ADD:
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value + rhs->value);

    case EJS_OP_AND:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) & fixed(rhs->value)));

    case EJS_OP_DIV:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
#endif
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value / rhs->value);

    case EJS_OP_MUL:
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value * rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) | fixed(rhs->value)));

    case EJS_OP_REM:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) % fixed(rhs->value)));
#else
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) % fixed(rhs->value)));
#endif

    case EJS_OP_SHL:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) << fixed(rhs->value)));

    case EJS_OP_SHR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_SUB:
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value - rhs->value);

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) ^ fixed(rhs->value)));

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
}


/*
 *  Number constructor.
 *
 *      function Number()
 *      function Number(value)
 */

static EjsVar *numberConstructor(Ejs *ejs, EjsNumber *np, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsNumber   *num;

    mprAssert(argc == 0 || (argc == 1 && ejsIsArray(argv[0])));

    if (argc == 1) {
        args = (EjsArray*) argv[0];

        num = ejsToNumber(ejs, ejsGetProperty(ejs, (EjsVar*) args, 0));
        if (num) {
            np->value = num->value;
        }
    }
    return (EjsVar*) np;
}


/*
 *  Function to iterate and return each number in sequence.
 *  NOTE: this is not a method of Number. Rather, it is a callback function for Iterator.
 */
static EjsVar *nextNumber(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsNumber   *np;

    np = (EjsNumber*) ip->target;
    if (!ejsIsNumber(np)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < np->value) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getNumberIterator(Ejs *ejs, EjsVar *np, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, np, (EjsNativeFunction) nextNumber, 0, NULL);
}



#ifndef ejsIsNan
int ejsIsNan(double f)
{
#if BLD_FEATURE_FLOATING_POINT
#if WIN
    return _isnan(f);
#elif VXWORKS
    /* TODO */
    return 0;
#else
    return (f == FP_NAN);
#endif
#else
    return 0;
#endif
}
#endif


bool ejsIsInfinite(MprNumber f)
{
#if BLD_FEATURE_FLOATING_POINT
#if WIN
    return !_finite(f);
#elif VXWORKS
    /* TODO */
    return 0;
#else
    return (f == FP_INFINITE);
#endif
#else
    return 0;
#endif
}

/*
 *  Create an initialized number
 */

EjsNumber *ejsCreateNumber(Ejs *ejs, MprNumber value)
{
    EjsNumber   *vp;

    if (value == 0) {
        return ejs->zeroValue;
    } else if (value == 1) {
        return ejs->oneValue;
    } else if (value == -1) {
        return ejs->minusOneValue;
    }

    vp = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    if (vp != 0) {
        vp->value = value;
    }

    ejsSetDebugName(vp, "number value");

    return vp;
}


void ejsCreateNumberType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;
#if BLD_FEATURE_FLOATING_POINT
    static int  zero = 0;
#endif

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Number"), ejs->objectType, sizeof(EjsNumber),
        ES_Number, ES_Number_NUM_CLASS_PROP, ES_Number_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->numberType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castNumber;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeNumberOperator;

    ejs->zeroValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->zeroValue->value = 0;
    ejs->oneValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->oneValue->value = 1;
    ejs->minusOneValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->minusOneValue->value = -1;

#if BLD_FEATURE_FLOATING_POINT
    ejs->infinityValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->infinityValue->value = 1.0 / zero;
    ejs->negativeInfinityValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->negativeInfinityValue->value = -1.0 / zero;
    ejs->nanValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->nanValue->value = 0.0 / zero;

    ejs->maxValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->maxValue->value = 1.7976931348623157e+308;
    ejs->minValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->minValue->value = 5e-324;

    ejsSetDebugName(ejs->infinityValue, "Infinity");
    ejsSetDebugName(ejs->negativeInfinityValue, "NegativeInfinity");
    ejsSetDebugName(ejs->nanValue, "NaN");

#else
    ejs->maxValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->maxValue->value = MAXINT;
    ejs->minValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->minValue->value = -MAXINT;
    ejs->nanValue = ejs->zeroValue;
#endif

    ejsSetDebugName(ejs->minusOneValue, "-1");
    ejsSetDebugName(ejs->oneValue, "1");
    ejsSetDebugName(ejs->zeroValue, "0");
    ejsSetDebugName(ejs->maxValue, "MaxValue");
    ejsSetDebugName(ejs->minValue, "MinValue");
}


void ejsConfigureNumberType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->numberType;

    ejsSetProperty(ejs, (EjsVar*) type, ES_Number_MaxValue, (EjsVar*) ejs->maxValue);
    ejsSetProperty(ejs, (EjsVar*) type, ES_Number_MinValue, (EjsVar*) ejs->minValue);
    ejsBindMethod(ejs, type, ES_Number_Number, (EjsNativeFunction) numberConstructor);
    ejsBindMethod(ejs, type, ES_Object_get, getNumberIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getNumberIterator);

    ejsSetProperty(ejs, ejs->global, ES_NegativeInfinity, (EjsVar*) ejs->negativeInfinityValue);
    ejsSetProperty(ejs, ejs->global, ES_Infinity, (EjsVar*) ejs->infinityValue);
    ejsSetProperty(ejs, ejs->global, ES_NaN, (EjsVar*) ejs->nanValue);
    ejsSetProperty(ejs, ejs->global, ES_num, (EjsVar*) type);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsNumber.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsObject.c"
 */
/************************************************************************/

/**
 *  ejsObject.c - Object class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static EjsName  getObjectPropertyName(Ejs *ejs, EjsObject *obj, int slotNum);
static int      growNames(EjsObject *obj, int size);
static int      growSlots(Ejs *ejs, EjsObject *obj, int size);
static int      hashProperty(EjsObject *obj, int slotNum, EjsName *qname);
static int      lookupObjectProperty(struct Ejs *ejs, EjsObject *obj, EjsName *qname);
static int      makeHash(EjsObject *obj);
static inline int cmpName(EjsName *a, EjsName *b);
static inline int cmpQname(EjsName *a, EjsName *b);
static void     removeHashEntry(EjsObject  *obj, EjsName *qname);
static EjsVar   *objectToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv);

#define CMP_NAME(a,b) cmpName(a, b)
#define CMP_QNAME(a,b) cmpQname(a, b)

/*
 *  Cast the operand to a primitive type
 *
 *  intrinsic function cast(type: Type) : Object
 */
static EjsVar *castObject(Ejs *ejs, EjsObject *obj, EjsType *type)
{
    EjsString   *result;
    char        *buf;
    
    mprAssert(ejsIsType(type));

    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_Number:
        result = ejsToString(ejs, (EjsVar*) obj);
        if (result == 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
        return ejsParseVar(ejs, ejsGetString(result), ES_Number);

    case ES_String:
        mprAllocSprintf(ejs, &buf, 0, "[object %s]", obj->var.type->qname.name);
        result = ejsCreateString(ejs, buf);
        mprFree(buf);
        return (EjsVar*) result;

    default:
        if (ejsIsA(ejs, (EjsVar*) obj, type)) {
            return (EjsVar*) obj;
        }
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


EjsObject *ejsCreateSimpleObject(Ejs *ejs)
{
    //  TODO - should this not be dynamic?
    return ejsCreateObjectEx(ejs, ejs->objectType, 0, 0);
}


EjsObject *ejsCreateObject(Ejs *ejs, EjsType *type, int numExtraSlots)
{
    return ejsCreateObjectEx(ejs, type, numExtraSlots, 0);
}


/*
 *  Create an object which is an instance of a given type. This is used by all scripted types to create objects. NOTE: 
 *  we only initialize the Object base class. It is up to the  caller to complete the initialization for all other base 
 *  classes by calling the appropriate constructors. capacity is the number of property slots to pre-allocate. Slots are 
 *  allocated and the property hash is configured. 
 */
EjsObject *ejsCreateObjectEx(Ejs *ejs, EjsType *type, int numExtraSlots, bool separateSlots)
{
    EjsObject   *obj;
    EjsBlock    *instanceBlock;
    int         numSlots, hashSize;

    mprAssert(type);
    mprAssert(numExtraSlots >= 0);

    instanceBlock = type->instanceBlock;
    numSlots = (instanceBlock ? instanceBlock->obj.numProp : 0) + numExtraSlots;
    mprAssert(numSlots >= 0);
    
    separateSlots |= type->separateInstanceSlots;

    /*
     *  Create the object from the variable allocator. We say how much room we need to reserve for property slots.
     */
    obj = (EjsObject*) ejsAllocVar(ejs, type, (separateSlots) ? 0 : (numSlots * sizeof(EjsVar*)));
    if (obj == 0) {
        return 0;
    }
    obj->var.isObject = 1;
    obj->var.dynamic = type->dynamicInstance;

    if (obj->var.dynamic) {
        mprAssert(separateSlots);
    }

    if (numSlots > 0) {
        if (separateSlots) {
            /*
             *  If the object requires separate slots then allocate the slots and property hash.
             */
            mprAssert(obj->numProp == 0);
            if (ejsGrowObject(ejs, obj, numSlots) < 0) {
                return 0;
            }
            mprAssert(obj->numProp == numSlots);

            if (instanceBlock && instanceBlock->obj.names) {
                
                if (instanceBlock->obj.names->sizeEntries) {
                    obj->names->sizeEntries = instanceBlock->obj.names->sizeEntries;
                    memcpy(obj->names->entries, instanceBlock->obj.names->entries, 
                        obj->names->sizeEntries * sizeof(EjsHashEntry));
                }
                if (instanceBlock->obj.names->buckets) {
                    hashSize = ejsGetHashSize(instanceBlock->obj.numProp);
                    obj->names->buckets = (int*) mprAlloc(obj->names, hashSize * sizeof(EjsHashEntry*));
                    if (obj->names->buckets == 0) {
                        return 0;
                    }
                    obj->names->sizeBuckets = instanceBlock->obj.names->sizeBuckets;
                    memcpy(obj->names->buckets, instanceBlock->obj.names->buckets, obj->names->sizeBuckets * sizeof(int*));
                }
            }
            
        } else {
            /*
             *  The slots are allocated as part of the object in a single memory chunk.
             */
            obj->slots = (EjsVar**) &(((char*) obj)[type->instanceSize]);
            ejsZeroSlots(ejs, obj->slots, numSlots);
            obj->capacity = numSlots;
            obj->numProp = numSlots;
            if (instanceBlock) {
                obj->names = instanceBlock->obj.names;
            }
        }
    }

    ejsSetFmtDebugName(obj, "obj %s", type->qname.name);

    return obj;
}


EjsObject *ejsCopyObject(Ejs *ejs, EjsObject *src, bool deep)
{
    EjsObject   *dest;
    bool        separateSlots;
    int         numProp, i;

    numProp = src->numProp;

    separateSlots = ejsIsType(src) ? ((EjsType*) src)->separateInstanceSlots: src->var.type->separateInstanceSlots;

    dest = ejsCreateObjectEx(ejs, src->var.type, numProp, separateSlots);
    if (dest == 0) {
        return 0;
    }
    
    /*
     *  Copy var flags but preserve generation. Don't copy rootLinks.
     *  TODO OPT - bit fields
     */
    dest->var.refLinks = src->var.refLinks;
    dest->var.builtin = src->var.builtin;
    dest->var.dynamic = src->var.dynamic;
    dest->var.hasGetterSetter = src->var.hasGetterSetter;
    dest->var.isFunction = src->var.isFunction;
    dest->var.isObject = src->var.isObject;
    dest->var.isInstanceBlock = src->var.isInstanceBlock;
    dest->var.isType = src->var.isType;
    dest->var.isFrame = src->var.isFrame;
    dest->var.hidden = src->var.hidden;
    dest->var.marked = src->var.marked;
    dest->var.native = src->var.native;
    dest->var.nativeProc = src->var.nativeProc;
    dest->var.permanent = src->var.permanent;
    dest->var.survived = src->var.survived;
    dest->var.visited = src->var.visited;

    
    ejsSetDebugName(dest, src->var.debugName);

    if (numProp <= 0) {
        return dest;
    }

    for (i = 0; i < numProp; i++) {
        if (deep) {
            dest->slots[i] = ejsCloneVar(ejs, src->slots[i], deep);
        } else {
            dest->slots[i] = src->slots[i];
        }
        ejsSetReference(ejs, (EjsVar*) dest, dest->slots[i]);
    }

    if (separateSlots) {
        if (dest->names == NULL && growNames(dest, numProp) < 0) {
            return 0;
        }

        for (i = 0; i < numProp; i++) {
            dest->names->entries[i] = src->names->entries[i];
        }
        if (makeHash(dest) < 0) {
            return 0;
        }

    } else {
        dest->names = src->names;
    }

    return dest;
}


/*
 *  Define a new property.
 */
static int defineObjectProperty(Ejs *ejs, EjsBlock *block, int slotNum, EjsName *qname, EjsType *propType, int attributes, 
    EjsVar *value)
{

    if (ejsIsBlock(block)) {
        return (ejs->blockHelpers->defineProperty)(ejs, (EjsVar*) block, slotNum, qname, propType, attributes, value);

    } else {
        ejsThrowInternalError(ejs, "Helper not defined for non-block object");
        return 0;
    }
}


/*
 *  Delete an instance property. To delete class properties, use the type as the obj.
 */
static int deleteObjectProperty(Ejs *ejs, EjsObject *obj, int slotNum)
{
    EjsName     qname;

    mprAssert(obj);
    mprAssert(obj->var.type);
    mprAssert(slotNum >= 0);

    if (!obj->var.dynamic && !(ejs->flags & EJS_FLAG_COMPILER)) {
        ejsThrowTypeError(ejs, "Can't delete properties in a non-dynamic object");
        return EJS_ERR;
    }

    if (slotNum < 0 || slotNum >= obj->numProp) {
        ejsThrowReferenceError(ejs, "Invalid property slot to delete");
        return EJS_ERR;
    }

    qname = getObjectPropertyName(ejs, obj, slotNum);
    if (qname.name == 0) {
        return EJS_ERR;
    }

    removeHashEntry(obj, &qname);
    obj->slots[slotNum] = 0;

    return 0;
}


/*
 *  Delete an instance property by name
 */
static int deleteObjectPropertyByName(Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    int     slotNum;

    slotNum = lookupObjectProperty(ejs, obj, qname);
    if (slotNum < 0) {
        ejsThrowReferenceError(ejs, "Property does not exist");
        return EJS_ERR;
    } else {
        return deleteObjectProperty(ejs, obj, slotNum);
    }
}


/*
 *  Object is being destroy. May be added to the type free list for recycling, so preserve the hash for non-dynamic objects.
 */
static void destroyObject(Ejs *ejs, EjsObject *obj)
{
    mprAssert(obj);

    if (obj->var.dynamic) {
        mprFree(obj->slots);
        obj->slots = 0;
        mprFree(obj->names);
        obj->names = 0;
    }
    
    ejsFreeVar(ejs, (EjsVar*) obj);
}


/**
 *  Finalizer hook for types. This should call a "finalize" method if one exists to release any internally allocated 
 *  resources. This routine will be called by the GC if it has not been previously called explicitly by the user. 
 *  So release is guaranteed to be called exactly and only once. For objects, release will remove all properties. 
 *  If called by the user, this call is a hint to GC that the object may be available for immmediate collection. 
 *  Not yet implemented.
 */
static int finalizeObject(Ejs *ejs, EjsObject *obj)
{
    //  TODO - Objects need an ability to define a user finalize() method.
    return 0;
}


static EjsVar *getObjectProperty(Ejs *ejs, EjsObject *obj, int slotNum)
{
    mprAssert(obj);
    mprAssert(obj->slots);
    mprAssert(slotNum >= 0);

    if (slotNum < 0 || slotNum >= obj->numProp) {
        ejsThrowReferenceError(ejs, "Property at slot \"%d\" is not found", slotNum);
        return 0;
    }
    return obj->slots[slotNum];
}


/*
 *  Return the number of properties in the object
 */
static int getObjectPropertyCount(Ejs *ejs, EjsObject *obj)
{
    mprAssert(obj);
    mprAssert(ejsIsObject(obj));

    return obj->numProp;
}


static EjsName getObjectPropertyName(Ejs *ejs, EjsObject *obj, int slotNum)
{
    EjsName     qname;

    mprAssert(obj);
    mprAssert(ejsIsObject(obj));
    mprAssert(obj->slots);
    mprAssert(slotNum >= 0);
    mprAssert(slotNum < obj->numProp);

    if (slotNum < 0 || slotNum >= obj->numProp) {
        qname.name = 0;
        qname.space = 0;
        return qname;
    }
    return obj->names->entries[slotNum].qname;
}


/*
 *  Cast the operands depending on the operation code
 */
EjsVar *ejsCoerceOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {

    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    case EJS_OP_COMPARE_EQ:  case EJS_OP_COMPARE_NE:
        if (ejsIsNull(rhs) || ejsIsUndefined(rhs)) {
            return (EjsVar*) ((opcode == EJS_OP_COMPARE_EQ) ? ejs->falseValue: ejs->trueValue);
        } else if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT:
        return 0;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


EjsVar *ejsObjectOperator(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = ejsCoerceOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, (lhs == rhs));

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs == rhs));

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->oneValue;

    /*
     *  Binary operators
     */
    case EJS_OP_ADD: case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL:
    case EJS_OP_REM: case EJS_OP_OR: case EJS_OP_SHL: case EJS_OP_SHR:
    case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


/*
 *  Lookup a property with a namespace qualifier in an object and return the slot if found. Return EJS_ERR if not found.
 */
static int lookupObjectProperty(struct Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    EjsNames    *names;
    EjsName     *propName;
    int         slotNum, index;

    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(qname->space);

    names = obj->names;

    if (names == 0) {
        return EJS_ERR;
    }

    if (names->buckets == 0) {
        /*
         *  No hash. Just do a linear search
         */
        for (slotNum = 0; slotNum < obj->numProp; slotNum++) {
            propName = &names->entries[slotNum].qname;
            if (CMP_QNAME(propName, qname)) {
                return slotNum;
            }
        }
        return EJS_ERR;
    }
    
    /*
     *  Find the property in the hash chain if it exists. Note the hash does not include the namespace portion.
     *  We assume that names rarely clash with different namespaces. We do this so variable lookup and do a one
     *  hash probe and find matching names. Lookup will then pick the right namespace.
     */
    index = ejsComputeHashCode(names, qname);

    for (slotNum = names->buckets[index]; slotNum >= 0;  slotNum = names->entries[slotNum].nextSlot) {
        propName = &names->entries[slotNum].qname;
        /*
         *  Compare the name including the namespace portion
         */
        if (CMP_QNAME(propName, qname)) {
            return slotNum;
        }
    }

    return EJS_ERR;
}


/*
 *  Lookup a qualified property name and count the number of name portion matches. This routine is used to quickly lookup a 
 *  qualified name AND determine if there are other names with different namespaces but having the same name portion.
 *  Returns EJS_ERR if more than one matching property is found (ie. two properties of the same name but with different 
 *  namespaces). This should be rare! Otherwise, return the slot number of the unique matching property.
 *
 *  This is a special lookup routine for fast varible lookup in the scope chain. Used by ejsLookupVar and ejsLookupScope.
 *  WARNING: updates qname->space
 */
int ejsLookupSingleProperty(Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    EjsNames    *names;
    EjsName     *propName;
    int         i, slotNum, index, count;

    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(qname->space);
    mprAssert(qname->space[0] == '\0');

    names = obj->names;
    slotNum = -1;
    count = 0;

    if (names) {
        if (names->buckets == 0) {
            /*
             *  No hash. Just do a linear search. Examine all properties.
             */
            for (i = 0; i < obj->numProp; i++) {
                propName = &names->entries[i].qname;
                if (CMP_NAME(propName, qname)) {
                    count++;
                    slotNum = i;
                }
            }

        } else {
            /*
             *  Find the property in the hash chain if it exists. Note the hash does NOT include the namespace portion.
             *  We assume that names rarely clash with different namespaces. We do this so variable lookup and a single hash 
             *  probe will find matching names.
             */
            index = ejsComputeHashCode(names, qname);

            for (i = names->buckets[index]; i >= 0;  i = names->entries[i].nextSlot) {
                propName = &names->entries[i].qname;
                if (CMP_NAME(propName, qname)) {
                    slotNum = i;
                    count++;
                }
            }
        }
        if (count == 1) {
            if (mprLookupHash(ejs->standardSpaces, names->entries[slotNum].qname.space)) {
                qname->space = names->entries[slotNum].qname.space;
            } else {
                slotNum = -2;
            }
        }
    }

    return (count <= 1) ? slotNum : -2;
}


/*
 *  Mark the object properties for the garbage collector
 */
void ejsMarkObject(Ejs *ejs, EjsVar *parent, EjsObject *obj)
{
    EjsType     *type;
    EjsVar      *vp;
    int         i;

    mprAssert(ejsIsObject(obj) || ejsIsBlock(obj) || ejsIsFunction(obj));

    type = obj->var.type;

    for (i = 0; i < obj->numProp; i++) {
        vp = obj->slots[i];
        if (vp == 0 || vp == ejs->nullValue || vp->generation == EJS_GEN_ETERNAL) {
            continue;
        }
        ejsMarkVar(ejs, (EjsVar*) obj, vp);
    }
}


/*
 *  Validate the supplied slot number. If set to -1, then return the next available property slot number.
 */
inline int ejsCheckObjSlot(Ejs *ejs, EjsObject *obj, int slotNum)
{
    if (slotNum < 0) {
        if (!obj->var.dynamic) {
            ejsThrowReferenceError(ejs, "Object is not dynamic");
            return EJS_ERR;
        }

        slotNum = obj->numProp;
        if (obj->numProp >= obj->capacity) {
            if (ejsGrowObject(ejs, obj, obj->numProp + 1) < 0) {
                ejsThrowMemoryError(ejs);
                return EJS_ERR;
            }
        } else {
            obj->numProp++;
        }

    } else if (slotNum >= obj->numProp) {
        if (ejsGrowObject(ejs, obj, slotNum + 1) < 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }
    }
    return slotNum;
}


/**
 *  Set the value of a property.
 *  @param slot If slot is -1, then allocate the next free slot
 *  @return Return the property slot if successful. Return < 0 otherwise.
 */
static int setObjectProperty(Ejs *ejs, EjsObject *obj, int slotNum, EjsVar *value)
{
    mprAssert(ejs);
    mprAssert(obj);
    
    if ((slotNum = ejsCheckObjSlot(ejs, obj, slotNum)) < 0) {
        return EJS_ERR;
    }
    
    mprAssert(slotNum < obj->numProp);
    mprAssert(obj->numProp <= obj->capacity);
    
    obj->slots[slotNum] = value;
    ejsSetReference(ejs, (EjsVar*) obj, value);

    return slotNum;
}


/*
 *  Set the name for a property. Objects maintain a hash lookup for property names. This is hash is created on demand 
 *  if there are more than N properties. If an object is not dynamic, it will use the types name hash. If dynamic, 
 *  then the types name hash will be copied when required. Callers must supply persistent names strings in qname. 
 */
static int setObjectPropertyName(Ejs *ejs, EjsObject *obj, int slotNum, EjsName *qname)
{
    EjsNames    *names;

    mprAssert(obj);
    mprAssert(qname);
    mprAssert(slotNum >= 0);

    if ((slotNum = ejsCheckObjSlot(ejs, obj, slotNum)) < 0) {
        return EJS_ERR;
    }

    /*
     *  If the hash is owned by the base type and this is a dynamic object, we need a new hash dedicated to the object.
     */
    if (obj->names == NULL) {
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }

    } else if (obj->var.dynamic && obj->names == obj->var.type->block.obj.names) {
        /*
         *  Object is using the type's original names, must copy and use own names from here on.
         */
        obj->names = NULL;
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }

//  TODO - BUG - names may not be a context
    } else if (!ejsIsType(obj) && obj != mprGetParent(obj->names)) {
        /*
         *  This case occurs when a dynamic local var is created in a function frame.
         */
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }
        
    } else if (slotNum >= obj->names->sizeEntries) {
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }
    }

    names = obj->names;

    /*
     *  Remove the old hash entry if the name will change
     */
    if (names->entries[slotNum].nextSlot >= 0) {
        if (CMP_QNAME(&names->entries[slotNum].qname, qname)) {
            return slotNum;
        }
        removeHashEntry(obj, &names->entries[slotNum].qname);
    }

    /*
     *  Set the property name
     */
    names->entries[slotNum].qname = *qname;
    
    mprAssert(slotNum < obj->numProp);
    mprAssert(obj->numProp <= obj->capacity);
    
    if (obj->numProp <= EJS_HASH_MIN_PROP || qname->name == NULL) {
        return slotNum;
    }

    if (hashProperty(obj, slotNum, qname) < 0) {
        ejsThrowMemoryError(ejs);
        return EJS_ERR;
    }

    return slotNum;
}


/*
 *  Return a memory context that should be used for any dynamically allocated data. Using this memory context will ensure
 *  the memory is automatically freed and all destructors are called when the object is freed. If the object is not dynamic, then
 *  a context cannot be supplied and the caller will have to use the destroyVar helper and explicitly release memory.
 */
MprCtx ejsGetContext(EjsObject *obj)
{
    if (!obj->var.dynamic && obj->slots) {
        return obj->slots;
    }
    return 0;
}


//  TODO - rename MakePropertyDeletable
void ejsMakePropertyDontDelete(EjsVar *vp, int dontDelete)
{
#if FUTURE
    vp->preventDelete = dontDelete;
#endif
}


/*
 *  Set a property's enumerability by for/in. Return true if the property was enumerable.
 */

int ejsMakePropertyEnumerable(EjsVar *vp, bool enumerate)
{
    int     oldValue;

    oldValue = vp->hidden;
    vp->hidden = !enumerate;
    return oldValue;
}


#if FUTURE
/*
 *  Make a variable read only. Can still be deleted.
 */
void ejsMakePropertyReadOnly(EjsVar *vp, int readonly)
{
    vp->readonly = readonly;
}
#endif


void ejsSetAllocIncrement(Ejs *ejs, EjsType *type, int num)
{
    if (type == 0) {
        return;
    }
    type->numAlloc = num;
}


/*
 *  Grow the slot storage for the object and increase numProp
 */
int ejsGrowObject(Ejs *ejs, EjsObject *obj, int count)
{
    int     size;
    
    if (count <= 0) {
        return 0;
    }

    if (obj->capacity < count) {
        size = EJS_PROP_ROUNDUP(count);

        if (growNames(obj, size) < 0) {
            return EJS_ERR;
        }
        if (growSlots(ejs, obj, size) < 0) {
            return EJS_ERR;
        }
        if (obj->numProp > 0 && makeHash(obj) < 0) {
            return EJS_ERR;
        }
    }

    if (count > obj->numProp) {
        obj->numProp = count;
    }
    
    mprAssert(count <= obj->capacity);
    mprAssert(obj->numProp <= obj->capacity);
    
    return 0;
}


/*
 *  Insert new slots at the specified offset and move up slots to make room. Increase numProp.
 */
int ejsInsertGrowObject(Ejs *ejs, EjsObject *obj, int incr, int offset)
{
    EjsHashEntry    *entries;
    EjsNames        *names;
    int             i, size, mark;

    mprAssert(obj);
    mprAssert(incr >= 0);

    if (incr == 0) {
        return 0;
    }
    
    /*
     *  Base this comparison on numProp and not on capacity as we may already have room to fit the inserted properties.
     */
    size = obj->numProp + incr;

    if (obj->capacity < size) {
        size = EJS_PROP_ROUNDUP(size);
        if (growNames(obj, size) < 0) {
            return EJS_ERR;
        }
        if (growSlots(ejs, obj, size) < 0) {
            return EJS_ERR;
        }
    }
    obj->numProp += incr;
    
    mprAssert(obj->capacity == obj->names->sizeEntries);
    mprAssert(obj->numProp <= obj->capacity);
    
    names = obj->names;
    mark = offset + incr;
    for (i = obj->numProp - 1; i >= mark; i--) {
        obj->slots[i] = obj->slots[i - mark];
        names->entries[i] = names->entries[i - mark];
    }

    ejsZeroSlots(ejs, &obj->slots[offset], incr);

    entries = names->entries;
    for (i = offset; i < mark; i++) {
        entries[i].nextSlot = -1;
        entries[i].qname.name = "";
        entries[i].qname.space = "";
    }

    if (makeHash(obj) < 0) {
        return EJS_ERR;
    }   
    
    return 0;
}


/*
 *  Allocate or grow the slots storage for an object
 */
static int growSlots(Ejs *ejs, EjsObject *obj, int capacity)
{
    int         factor;

    mprAssert(obj);

    if (capacity <= obj->capacity) {
        return 0;
    }

    /*
     *  Allocate or grow the slots structures
     */
    if (capacity > obj->capacity) {
        if (obj->capacity > EJS_LOTSA_PROP) {
            /*
             *  Looks like a big object so grow by a bigger chunk. TODO - should we also grow names?
             */
            factor = max(obj->capacity / 4, EJS_NUM_PROP);
            capacity = (capacity + factor) / factor * factor;
        }
        capacity = EJS_PROP_ROUNDUP(capacity);

        if (obj->slots == 0) {
            mprAssert(obj->capacity == 0);
            mprAssert(capacity > 0);
            obj->slots = (EjsVar**) mprAlloc(obj, sizeof(EjsVar*) * capacity);
            if (obj->slots == 0) {
                return EJS_ERR;
            }
            ejsZeroSlots(ejs, obj->slots, capacity);

        } else {
            mprAssert(obj->capacity > 0);
            obj->slots = (EjsVar**) mprRealloc(obj, obj->slots, sizeof(EjsVar*) * capacity);
            if (obj->slots == 0) {
                return EJS_ERR;
            }
            ejsZeroSlots(ejs, &obj->slots[obj->capacity], (capacity - obj->capacity));
        }
        obj->capacity = capacity;
    }

    return 0;
}


/*
 *  Remove a slot and name. Copy up all other properties. WARNING: this can only be used before property binding and 
 *  should only be used by the compiler.
 */
void ejsRemoveSlot(Ejs *ejs, EjsObject *obj, int slotNum, int compact)
{
    EjsNames    *names;
    int         i;

    mprAssert(obj);
    mprAssert(slotNum >= 0);
    mprAssert(slotNum >= 0);
    mprAssert(ejs->flags & EJS_FLAG_COMPILER);

    names = obj->names;

    if (compact) {
        mprAssert(names);

        for (i = slotNum + 1; i < obj->numProp; i++) {
            obj->slots[i - 1] = obj->slots[i];
            names->entries[i - 1] = names->entries[i];
        }
        obj->numProp--;
        i--;

    } else {
        i = slotNum;
    }

    obj->slots[i] = 0;
    names->entries[i].qname.name = "";
    names->entries[i].qname.space = "";
    names->entries[i].nextSlot = -1;
    
    makeHash(obj);
}



/*
 *  Exponential primes
 */
static int hashSizes[] = {
     19, 29, 59, 79, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 0
};


int ejsGetHashSize(int numProp)
{
    int     i;

    for (i = 0; i < hashSizes[i]; i++) {
        if (numProp < hashSizes[i]) {
            return hashSizes[i];
        }
    }
    return hashSizes[i - 1];
}


/*
 *  Grow the names vector
 */
static int growNames(EjsObject *obj, int size)
{
    EjsNames        *names;
    EjsHashEntry    *entries;
    bool            ownNames;
    int             i, oldSize;

    if (size == 0) {
        return 0;
    }

    names = obj->names;
    ownNames = (obj == mprGetParent(names));
    oldSize = (names) ? names->sizeEntries: 0;

    if (names == NULL || !ownNames) {
        names = mprAllocObj(obj, EjsNames);
        if (names == 0) {
            return EJS_ERR;
        }
        names->buckets = 0;
        names->entries = 0;
        names->sizeEntries = 0;
        names->sizeBuckets = 0;
    }

    if (size < names->sizeEntries) {
        return 0;
    }
    size = EJS_PROP_ROUNDUP(size);
    
    if (ownNames) {
        entries = (EjsHashEntry*) mprRealloc(names, names->entries, sizeof(EjsHashEntry) * size);
        if (entries == 0) {
            return EJS_ERR;
        }

    } else {
        entries = (EjsHashEntry*) mprAlloc(names, sizeof(EjsHashEntry) * size);
        if (entries == 0) {
            return EJS_ERR;
        }
        if (obj->names) {
            for (i = 0; i < oldSize; i++) {
                entries[i] = obj->names->entries[i];
            }
        }
    }

    for (i = oldSize; i < size; i++) {
        entries[i].nextSlot = -1;
        entries[i].qname.name = "";
        entries[i].qname.space = "";
    }
                
    names->sizeEntries = size;
    names->entries = entries;
    obj->names = names;

    return 0;
}


static int hashProperty(EjsObject *obj, int slotNum, EjsName *qname)
{
    EjsNames    *names;
    EjsName     *slotName;
    int         chainSlotNum, lastSlot, index;

    mprAssert(qname);

    names = obj->names;
    mprAssert(names);
  
    /*
     *  Test if the number of hash buckets is too small or non-existant and re-make the hash.
     */
    if (names->sizeBuckets < obj->numProp) {
        return makeHash(obj);
    }

    index = ejsComputeHashCode(names, qname);

    /*
     *  Scan the collision chain
     */
    lastSlot = -1;
    chainSlotNum = names->buckets[index];
    mprAssert(chainSlotNum < obj->numProp);
    mprAssert(chainSlotNum < obj->capacity);

    while (chainSlotNum >= 0) {
        slotName = &names->entries[chainSlotNum].qname;
        if (CMP_QNAME(slotName, qname)) {
            return 0;
        }
        mprAssert(lastSlot != chainSlotNum);
        lastSlot = chainSlotNum;
        mprAssert(chainSlotNum != names->entries[chainSlotNum].nextSlot);
        chainSlotNum = names->entries[chainSlotNum].nextSlot;

        mprAssert(0 <= lastSlot && lastSlot < obj->numProp);
        mprAssert(0 <= lastSlot && lastSlot < obj->capacity);
    }

    if (lastSlot >= 0) {
        mprAssert(lastSlot < obj->numProp);
        mprAssert(lastSlot != slotNum);
        names->entries[lastSlot].nextSlot = slotNum;

    } else {
        /* Start a new hash chain */
        names->buckets[index] = slotNum;
    }

    names->entries[slotNum].nextSlot = -2;
    names->entries[slotNum].qname = *qname;

#if BLD_DEBUG
    if (obj->slots[slotNum] && obj->slots[slotNum]->debugName[0] == '\0') {
        ejsSetDebugName(obj->slots[slotNum], qname->name);
    }
#endif

    return 0;
}


/*
 *  Allocate or grow the properties storage for an object. This routine will also manage the hash index for the object. 
 *  If numInstanceProp is < 0, then grow the number of properties by an increment. Otherwise, set the number of properties 
 *  to numInstanceProp. We currently don't allow reductions.
 */
static int makeHash(EjsObject *obj)
{
    EjsHashEntry    *entries;
    EjsNames        *names;
    int             i, newHashSize;

    mprAssert(obj);

    names = obj->names;

    /*
     *  Don't make the hash if too few properties. Once we have a hash, keep using it even if we have too few properties now.
     */
    if (obj->numProp <= EJS_HASH_MIN_PROP && names->buckets == 0) {
        return 0;
    }

    /*
     *  Only reallocate the hash buckets if the hash needs to grow larger
     */
    newHashSize = ejsGetHashSize(obj->numProp);
    if (names->sizeBuckets < newHashSize) {
        mprFree(names->buckets);
        names->buckets = (int*) mprAlloc(names, newHashSize * sizeof(int));
        if (names->buckets == 0) {
            return EJS_ERR;
        }
        names->sizeBuckets = newHashSize;
    }
    mprAssert(names->buckets);

    /*
     *  Clear out hash linkage
     */
    memset(names->buckets, -1, names->sizeBuckets * sizeof(int));
    entries = names->entries;
    for (i = 0; i < names->sizeEntries; i++) {
        entries[i].nextSlot = -1;
    }

    /*
     *  Rehash all existing properties
     */
    for (i = 0; i < obj->numProp; i++) {
        if (entries[i].qname.name && hashProperty(obj, i, &entries[i].qname) < 0) {
            return EJS_ERR;
        }
    }

    return 0;
}


static void removeHashEntry(EjsObject *obj, EjsName *qname)
{
    EjsNames        *names;
    EjsHashEntry    *he;
    EjsName         *nextName;
    int             index, slotNum, lastSlot;

    names = obj->names;
    if (names == 0) {
        return;
    }

    if (names->buckets == 0) {
        /*
         *  No hash. Just do a linear search
         */
        for (slotNum = 0; slotNum < obj->numProp; slotNum++) {
            he = &names->entries[slotNum];
            if (CMP_QNAME(&he->qname, qname)) {
                he->qname.name = "";
                he->qname.space = "";
                he->nextSlot = -1;
                return;
            }
        }
        mprAssert(0);
        return;
    }


    index = ejsComputeHashCode(names, qname);
    slotNum = names->buckets[index];
    lastSlot = -1;
    while (slotNum >= 0) {
        he = &names->entries[slotNum];
        nextName = &he->qname;
        if (CMP_QNAME(nextName, qname)) {
            if (lastSlot >= 0) {
                names->entries[lastSlot].nextSlot = names->entries[slotNum].nextSlot;
            } else {
                names->buckets[index] = names->entries[slotNum].nextSlot;
            }
            he->qname.name = "";
            he->qname.space = "";
            he->nextSlot = -1;
            return;
        }
        lastSlot = slotNum;
        slotNum = names->entries[slotNum].nextSlot;
    }
    mprAssert(0);
}


int ejsRebuildHash(Ejs *ejs, EjsObject *obj)
{
    return makeHash(obj);
}


/*
 *  Compute a property name hash. Based on work by Paul Hsieh.
 */
int ejsComputeHashCode(EjsNames *names, EjsName *qname)
{
    ushort  *data;
    uchar   *cdata;
    uint    len, hash, rem, tmp;

    mprAssert(names);
    mprAssert(qname);
    mprAssert(qname->name);

    data = (ushort*) qname->name;
    len = (int) strlen(qname->name);

    if (len == 0) {
        return 0;
    }

    /*
     *  Do 32-bit wide computation
     */
    rem = len & 3;
    hash = len;

    for (len >>= 2; len > 0; len--, data += 2) {
        hash  += *data;
        tmp   =  (data[1] << 11) ^ hash;
        hash  =  (hash << 16) ^ tmp;
        hash  += hash >> 11;
    }

    /* Handle end cases */
    cdata = (uchar*) data;
    switch (rem) {
    case 3: 
        hash += cdata[0] + (cdata[1] << 8);
        hash ^= hash << 16;
        hash ^= cdata[sizeof(ushort)] << 18;
        hash += hash >> 11;
        break;
    case 2: 
        hash += cdata[0] + (cdata[1] << 8);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += cdata[0];
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* 
     *  Force "avalanching" of final 127 bits 
     */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    mprAssert(names->sizeBuckets);
    
    return hash % names->sizeBuckets;
}


static inline int cmpQname(EjsName *a, EjsName *b) 
{
    mprAssert(a);
    mprAssert(b);
    mprAssert(a->name);
    mprAssert(a->space);
    mprAssert(b->name);
    mprAssert(b->space);

    if (a->name == b->name && a->space == b->space) {
        return 1;
    }
    if (a->name[0] == b->name[0] && strcmp(a->name, b->name) == 0) {
        if (a->space[0] == b->space[0] && strcmp(a->space, b->space) == 0) {
            return 1;
        }
    }
    return 0;
}


static inline int cmpName(EjsName *a, EjsName *b) 
{
    mprAssert(a);
    mprAssert(b);
    mprAssert(a->name);
    mprAssert(b->name);

    if (a->name == b->name) {
        return 1;
    }
    if (a->name[0] == b->name[0] && strcmp(a->name, b->name) == 0) {
        return 1;
    }
    return 0;
}


/*
 *  WARNING: All methods here may be invoked by Native classes who are based on EjsVar and not on EjsObject. Because 
 *  all classes subclass Object, they need to be able to use these methods. So these methods must use the generic helper 
 *  interface (see ejsVar.h). They MUST NOT use EjsObject internals.
 */

static EjsVar *cloneObjectMethod(Ejs *ejs, EjsVar *op, int argc, EjsVar **argv)
{
    bool    deep;

    deep = (argc == 1 && argv[0] == (EjsVar*) ejs->trueValue);

    return ejsCloneVar(ejs, op, deep);
}


/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Object. Rather, it is a callback function for Iterator.
 */
static EjsVar *nextObjectKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsObject   *obj;
    EjsName     qname;

    obj = (EjsObject*) ip->target;
    if (!ejsIsObject(obj)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < obj->numProp; ip->index++) {
        qname = ejsGetPropertyName(ejs, (EjsVar*) obj, ip->index);
        if (qname.name == 0) {
            continue;
        }
        /*
         *  Enumerate over properties that have a public public or "" namespace 
         */
        if (qname.space[0] && strcmp(qname.space, EJS_PUBLIC_NAMESPACE) != 0) {
            continue;
        }
        ip->index++;
        //  TODO - what about returning a Name?
        return (EjsVar*) ejsCreateString(ejs, qname.name);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator.
 *
 *  iterator native function get(deep: Boolean = false, namespaces: Array = null): Iterator
 */
static EjsVar *getObjectIterator(Ejs *ejs, EjsVar *op, int argc, EjsVar **argv)
{
    EjsVar      *namespaces;
    bool        deep;

    deep = (argc == 1) ? ejsGetBoolean(argv[0]): 0;
    namespaces =  (argc == 2) ? argv[1]: 0;

    return (EjsVar*) ejsCreateIterator(ejs, op, (EjsNativeFunction) nextObjectKey, deep, (EjsArray*) namespaces);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Object. Rather, it is a callback function for Iterator
 */
static EjsVar *nextObjectValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsObject   *obj;
    EjsVar      *vp;
    EjsName     qname;

    obj = (EjsObject*) ip->target;
    if (!ejsIsObject(obj)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < obj->numProp; ip->index++) {
        qname = ejsGetPropertyName(ejs, (EjsVar*) obj, ip->index);
        if (qname.name == 0) {
            continue;
        }
        if (qname.space[0] && strcmp(qname.space, EJS_PUBLIC_NAMESPACE) != 0) {
            continue;
        }
        vp = obj->slots[ip->index];
        if (vp) {
            ip->index++;
            return vp;
        }
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(deep: Boolean = false, namespaces: Array = null): Iterator
 */
static EjsVar *getObjectValues(Ejs *ejs, EjsVar *op, int argc, EjsVar **argv)
{
    EjsVar      *namespaces;
    bool        deep;

    deep = (argc == 1) ? ejsGetBoolean(argv[0]): 0;
    namespaces =  (argc == 2) ? argv[1]: 0;

    return (EjsVar*) ejsCreateIterator(ejs, op, (EjsNativeFunction) nextObjectValue, deep, (EjsArray*) namespaces);
}


/*
 *  Get the length for the object.
 *
 *  intrinsic function get length(): Number
 */
static EjsVar *getObjectLength(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ejsGetPropertyCount(ejs, vp));
}


#if ES_Object_propertyIsEnumerable && FUTURE
/**
 *  Test and optionally set the enumerability flag for a property.
 *  intrinsic native function propertyIsEnumerable(property: String, flag: Object = undefined): Boolean
 */
static EjsVar *propertyIsEnumerable(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    EjsVar      *pp;

    mprAssert(argc == 1 || argc == 2);
    pp = getObjectProperty(ejs, vp, argv[0]);
    if (pp == 0) {
        return ejs->falseValue;
    }

    if (argc == 2) {
        if (ejsIsBoolean(argv[1])) {
            pp->hidden = (((EjsBoolean*) argv[1])->value);
        }
    }

    if (pp->hidden) {
        return ejs->falseValue;
    }
    return ejs->trueValue;
}
#endif


#if ES_Object_seal
/**
 *  Seal a dynamic object. Once an object is sealed, further attempts to create or delete properties will fail and will throw 
 *  @spec ejs-11
 */
static EjsVar *seal(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    //  TODO - not implemented or used anywhere
    vp->sealed = 1;
    return 0;
}
#endif


#if ES_Object_toLocaleString
/*
 *  Convert the object to a localized string.
 *
 *  intrinsic function toLocaleString(): String
 *
 *  TODO - currently just calls toString
 */
static EjsVar *toLocaleString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return objectToString(ejs, vp, argc, argv);
}
#endif


/*
 *  Convert the object to a string.
 *
 *  intrinsic function toString(): String
 */
static EjsVar *objectToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsToString(ejs, vp);
}


/*
 *  Create the object type
 */
void ejsCreateObjectType(Ejs *ejs)
{
    EjsName     qname;

    /*
     *  As instances based on pure "Object" are dynamic, we store the slots separately. So we create
     *  instances of size == sizeof(EjsObject).
     */
    ejs->objectType = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Object"), 0, sizeof(EjsObject), 
        ES_Object, ES_Object_NUM_CLASS_PROP, ES_Object_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureObjectType(Ejs *ejs)
{
    EjsType     *type, *t2;
    EjsFunction *fun, *existingFun;
    int         count, i, j;

    type = ejs->objectType;
    mprAssert(type);

    ejsBindMethod(ejs, type, ES_Object_clone, cloneObjectMethod);
    ejsBindMethod(ejs, type, ES_Object_get, getObjectIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getObjectValues);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_Object_length, getObjectLength, -1, 0);
#else
    ejsBindMethod(ejs, type, ES_Object_length, getObjectLength);
#endif
    ejsBindMethod(ejs, type, ES_Object_toString, objectToString);

#if ES_Object_propertyIsEnumerable && FUTURE
    ejsBindMethod(ejs, type, ES_Object_propertyIsEnumerable, propertyIsEnumerable);
#endif
#if ES_Object_seal
    ejsBindMethod(ejs, type, ES_Object_seal, seal);
#endif
#if ES_Object_toLocaleString
    ejsBindMethod(ejs, type, ES_Object_toLocaleString, toLocaleString);
#endif

#if FUTURE
    ejsBindMethod(ejs, type, ES_Object___defineProperty__, __defineProperty__);
    ejsBindMethod(ejs, type, ES_Object_hasOwnProperty, hasOwnProperty);
    ejsBindMethod(ejs, type, ES_Object_isPrototypeOf, isPrototypeOf);
#endif

    /*
     *  Patch native methods into all objects inheriting from object
     *  TODO - generalize and move into ejsBlock
     */
    count = ejsGetPropertyCount(ejs, ejs->global);
    for (i = 0; i < count; i++) {
        t2 = (EjsType*) ejsGetProperty(ejs, ejs->global, i);
        if (t2 != type && ejsIsType(t2) && !t2->isInterface && t2->hasObject) {
            for (j = 0; j < type->block.obj.numProp; j++) {
                fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, j);
                if (ejsIsNativeFunction(fun)) {
                    existingFun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) t2, j);
                    if (!ejsIsFunction(existingFun) || !existingFun->override) {
                        ejsSetProperty(ejs, (EjsVar*) t2, j, (EjsVar*) fun);
                    }
                }
            }
        }
    }
}


void ejsInitializeObjectHelpers(EjsTypeHelpers *helpers)
{
    /*
     *  call is not implemented, EjsObject does not override and it is handled by the vm.
     */
    helpers->castVar                = (EjsCastVarHelper) castObject;
    helpers->cloneVar               = (EjsCloneVarHelper) ejsCopyObject;
    helpers->createVar              = (EjsCreateVarHelper) ejsCreateObject;
    helpers->defineProperty         = (EjsDefinePropertyHelper) defineObjectProperty;
    helpers->destroyVar             = (EjsDestroyVarHelper) destroyObject;
    helpers->deleteProperty         = (EjsDeletePropertyHelper) deleteObjectProperty;
    helpers->deletePropertyByName   = (EjsDeletePropertyByNameHelper) deleteObjectPropertyByName;
    helpers->finalizeVar            = (EjsFinalizeVarHelper) finalizeObject;
    helpers->getProperty            = (EjsGetPropertyHelper) getObjectProperty;
    helpers->getPropertyCount       = (EjsGetPropertyCountHelper) getObjectPropertyCount;
    helpers->getPropertyName        = (EjsGetPropertyNameHelper) getObjectPropertyName;
    helpers->lookupProperty         = (EjsLookupPropertyHelper) lookupObjectProperty;
    helpers->invokeOperator         = (EjsInvokeOperatorHelper) ejsObjectOperator;
    helpers->markVar                = (EjsMarkVarHelper) ejsMarkObject;
    helpers->setProperty            = (EjsSetPropertyHelper) setObjectProperty;
    helpers->setPropertyName        = (EjsSetPropertyNameHelper) setObjectPropertyName;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsObject.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsReflect.c"
 */
/************************************************************************/

/**
 *  ejsReflect.c - Reflection class and API
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if ES_Reflect
/*
 *  Constructor
 *
 *  public function Reflect(o: Object)
 */
static EjsVar *reflectConstructor(Ejs *ejs, EjsReflect *rp, int argc,  EjsVar **argv)
{
    mprAssert(argc == 1);
    rp->subject = argv[0];
    return (EjsVar*) rp;
}


/*
 *  Get the name of the subject
 *
 *  function get name(): String
 */
static EjsVar *getReflectedName(Ejs *ejs, EjsReflect *rp, int argc, EjsVar **argv)
{
    EjsFunction     *fun;
    EjsName         qname;
    EjsVar      *vp;

    mprAssert(argc == 0);

    vp = rp->subject;

    if (ejsIsType(vp)) {
        return (EjsVar*) ejsCreateString(ejs, ((EjsType*) vp)->qname.name);

    } else if (ejsIsFunction(vp)) {
        fun = (EjsFunction*) vp;
        qname = ejsGetPropertyName(ejs, fun->owner, fun->slotNum);
        return (EjsVar*) ejsCreateString(ejs, qname.name);

    } else {
        return (EjsVar*) ejsCreateString(ejs, vp->type->qname.name);
    }
    return ejs->undefinedValue;
}


/*
 *  Get the type of the object.
 *
 *  function get type(): Object
 */
static EjsVar *getReflectedType(Ejs *ejs, EjsReflect *rp, int argc, EjsVar **argv)
{
    EjsType     *type;
    EjsVar      *vp;

    vp = rp->subject;

    if (ejsIsType(vp)) {
        type = (EjsType*) vp;
        if (type->baseType) {
            return (EjsVar*) type->baseType;
        } else {
            return (EjsVar*) ejs->undefinedValue;
        }
    }
    return (EjsVar*) vp->type;
}


/*
 *  Get the type name of the subject
 *
 *  function get typeName(): String
 */
static EjsVar *getReflectedTypeName(Ejs *ejs, EjsReflect *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsGetTypeName(ejs, rp->subject);
}


/*
 *  Return the type name of a var as a string. If the var is a type, get the base type.
 */
EjsVar *ejsGetTypeName(Ejs *ejs, EjsVar *vp)
{
    EjsType     *type;

    if (vp == 0) {
        return ejs->undefinedValue;
    }
    if (ejsIsType(vp)) {
        type = (EjsType*) vp;
        if (type->baseType) {
            return (EjsVar*) ejsCreateString(ejs, type->baseType->qname.name);
        } else {
            /* NOTE: the base type of Object is null */
            return (EjsVar*) ejs->nullValue;
        }
    }
    return (EjsVar*) ejsCreateString(ejs, vp->type->qname.name);
}


/*
 *  Get the ecma "typeof" value for an object. Unfortunately, typeof is pretty lame.
 */
EjsVar *ejsGetTypeOf(Ejs *ejs, EjsVar *vp)
{
    if (vp == ejs->undefinedValue) {
        return (EjsVar*) ejsCreateString(ejs, "undefined");

    } else if (vp == ejs->nullValue) {
        /* Yea - I know, ECMAScript is broken */
        return (EjsVar*) ejsCreateString(ejs, "object");

    } if (ejsIsBoolean(vp)) {
        return (EjsVar*) ejsCreateString(ejs, "boolean");

    } else if (ejsIsNumber(vp)) {
        return (EjsVar*) ejsCreateString(ejs, "number");

    } else if (ejsIsString(vp)) {
        return (EjsVar*) ejsCreateString(ejs, "string");

    } else if (ejsIsType(vp)) {
        /* Pretend it is a constructor function */
        return (EjsVar*) ejsCreateString(ejs, "function");
               
    } else {
        return (EjsVar*) ejsCreateString(ejs, "object");
    }
}



void ejsCreateReflectType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Reflect"), ejs->objectType, sizeof(EjsReflect),
        ES_Reflect, ES_Reflect_NUM_CLASS_PROP, ES_Reflect_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
}


void ejsConfigureReflectType(Ejs *ejs)
{
    EjsType     *type;

    type = ejsGetType(ejs, ES_Reflect);

    ejsBindMethod(ejs, type, type->block.numInherited, (EjsNativeFunction) reflectConstructor);
    ejsBindMethod(ejs, type, ES_Reflect_name, (EjsNativeFunction) getReflectedName);
    ejsBindMethod(ejs, type, ES_Reflect_type, (EjsNativeFunction) getReflectedType);
    ejsBindMethod(ejs, type, ES_Reflect_typeName, (EjsNativeFunction) getReflectedTypeName);
}
#endif /* ES_Reflect */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsReflect.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsRegExp.c"
 */
/************************************************************************/

/**
 *  ejsRegExp.c - RegExp type class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_REGEXP && ES_RegExp


static int parseFlags(EjsRegExp *rp, cchar *flags);

/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castRegExp(Ejs *ejs, EjsRegExp *rp, EjsType *type)
{
    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, rp->pattern);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }

    return 0;
}


static void destroyRegExp(Ejs *ejs, EjsRegExp *rp)
{
    mprAssert(rp);

    if (rp->compiled) {
        free(rp->compiled);
        rp->compiled = 0;
    }
    ejsFreeVar(ejs, (EjsVar*) rp);
}


/*
 *  RegExp constructor
 *
 *  RegExp(pattern: String, flags: String = null)
 */

static EjsVar *regexConstructor(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    cchar       *errMsg;
    char        *pattern, *flags, *barePattern;
    int         column, errCode, options;

    pattern = ejsGetString(argv[0]);
    options = PCRE_JAVASCRIPT_COMPAT;
    barePattern = 0;

    if (argc == 2) {
        flags = ejsGetString(argv[1]);
        options |= parseFlags(rp, flags);
        mprAllocStrcat(rp, &rp->pattern, -1, NULL, pattern, flags, NULL);

    } else {
        rp->pattern = mprStrdup(rp, pattern);
    }

    if (*pattern == '/') {
        barePattern = mprStrdup(rp, &pattern[1]);
        if ((flags = strrchr(barePattern, '/')) != 0) {
            *flags++ = '\0';
        }
        options |= parseFlags(rp, flags);
        pattern = barePattern;
    }

    rp->compiled = (void*) pcre_compile2(pattern, options, &errCode, &errMsg, &column, NULL);
    mprFree(barePattern);

    if (rp->compiled == NULL) {
        ejsThrowArgError(ejs, "Can't compile regular expression. Error %s at column %d", errMsg, column);
    }
    return (EjsVar*) rp;
}


static EjsVar *getLastIndex(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, rp->endLastMatch);
}


/*
 *  function set lastIndex(value: Number): Void
 */
static EjsVar *setLastIndex(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    rp->endLastMatch = (int) ejsGetNumber(argv[0]);
    return 0;
}


/*
 *  function exec(str: String, start: Number = 0): Array
 */
static EjsVar *exec(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    EjsArray    *results;
    EjsString   *match;
    cchar       *str;
    int         matches[EJS_MAX_REGEX_MATCHES * 3];
    int         start, options, len, i, count, index;

    str = ejsGetString(argv[0]);
    if (argc == 2) {
        start = (int) ejsGetNumber(argv[1]);
    } else {
        start = rp->endLastMatch;
    }
    options = 0;
    rp->matched = 0;

    count = pcre_exec(rp->compiled, NULL, str, (int) strlen(str), start, options, matches, sizeof(matches) / sizeof(int));
    if (count < 0) {
        rp->endLastMatch = 0;
        return (EjsVar*) ejs->nullValue;
    }

    results = ejsCreateArray(ejs, count);
    for (index = 0, i = 0; i < count; i++, index += 2) {
        len = matches[index + 1] - matches[index];
        match = ejsCreateStringWithLength(ejs, &str[matches[index]], len);
        ejsSetProperty(ejs, (EjsVar*) results, i, (EjsVar*) match);
        if (index == 0) {
            rp->matched = match;
        }
    }

    /*
     *  Save some of the results
     */
    rp->startLastMatch = matches[0];
    rp->endLastMatch = matches[1];

    return (EjsVar*) results;
}


static EjsVar *getGlobalFlag(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, rp->global);
}


static EjsVar *getIgnoreCase(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, rp->ignoreCase);
}


static EjsVar *getMultiline(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, rp->multiline);
}


static EjsVar *getSource(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, rp->pattern);
}


static EjsVar *matched(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    if (rp->matched == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) rp->matched;
}


static EjsVar *start(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, rp->startLastMatch);
}


static EjsVar *sticky(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, rp->sticky);
}


static EjsVar *test(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    cchar       *str;
    int         count;

    str = ejsGetString(argv[0]);
    count = pcre_exec(rp->compiled, NULL, str, (int) strlen(str), rp->endLastMatch, 0, 0, 0);
    if (count < 0) {
        rp->endLastMatch = 0;
        return (EjsVar*) ejs->falseValue;
    }

    return (EjsVar*) ejs->trueValue;
}


static EjsVar *regExpToString(Ejs *ejs, EjsRegExp *rp, int argc, EjsVar **argv)
{
    return 0;
}


/*
 *  Create an initialized regular expression object. The pattern should include
 *  the slash delimiters. For example: /abc/ or /abc/g
 */

EjsRegExp *ejsCreateRegExp(Ejs *ejs, cchar *pattern)
{
    EjsRegExp   *rp;
    cchar       *errMsg;
    char        *flags, *barePattern;
    int         column, errCode, options;

    mprAssert(pattern[0] == '/');

    rp = (EjsRegExp*) ejsCreateVar(ejs, ejs->regExpType, 0);
    if (rp != 0) {
        rp->pattern = mprStrdup(rp, pattern);

        /*
         *  Strip off flags for passing to pcre_compile2
         */
        mprAssert(pattern[0] == '/');
        barePattern = mprStrdup(rp, &pattern[1]);
        if ((flags = strrchr(barePattern, '/')) != 0) {
            *flags++ = '\0';
        }

        options = parseFlags(rp, flags);
        rp->compiled = pcre_compile2(barePattern, options, &errCode, &errMsg, &column, NULL);
        mprFree(barePattern);

        if (rp->compiled == NULL) {
            ejsThrowArgError(ejs, "Can't compile regular expression. Error %s at column %d", errMsg, column);
            return 0;
        }
    }
    return rp;
}


static int parseFlags(EjsRegExp *rp, cchar *flags)
{
    cchar       *cp;
    int         options;

    if (flags == 0 || *flags == '\0') {
        return 0;
    }

    options = PCRE_JAVASCRIPT_COMPAT;
    for (cp = flags; *cp; cp++) {
        switch (tolower((int) *cp)) {
        case 'g':
            rp->global = 1;
            break;
        case 'i':
            rp->ignoreCase = 1;
            options |= PCRE_CASELESS;
            break;
        case 'm':
            rp->multiline = 1;
            options |= PCRE_MULTILINE;
            break;
        case 'y':
            rp->sticky = 1;
            break;
        }
    }
    return options;
}


void ejsCreateRegExpType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "RegExp"), ejs->objectType, sizeof(EjsRegExp),
        ES_RegExp, ES_RegExp_NUM_CLASS_PROP, ES_RegExp_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->regExpType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castRegExp;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyRegExp;
}


void ejsConfigureRegExpType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->regExpType;

    ejsBindMethod(ejs, type, ES_RegExp_RegExp, (EjsNativeFunction) regexConstructor);
    ejsBindMethod(ejs, type, ES_RegExp_exec, (EjsNativeFunction) exec);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_RegExp_lastIndex, (EjsNativeFunction) getLastIndex, ES_RegExp_set_lastIndex, 
        (EjsNativeFunction) setLastIndex);
    ejsSetAccessors(ejs, type, ES_RegExp_global, (EjsNativeFunction) getGlobalFlag, -1, 0);
    ejsSetAccessors(ejs, type, ES_RegExp_ignoreCase, (EjsNativeFunction) getIgnoreCase, -1, 0);
    ejsSetAccessors(ejs, type, ES_RegExp_multiline, (EjsNativeFunction) getMultiline, -1, 0);
    ejsSetAccessors(ejs, type, ES_RegExp_source, (EjsNativeFunction) getSource, -1, 0);
    ejsSetAccessors(ejs, type, ES_RegExp_matched, (EjsNativeFunction) matched, -1, 0);
    ejsSetAccessors(ejs, type, ES_RegExp_start, (EjsNativeFunction) start, -1, 0);
    ejsSetAccessors(ejs, type, ES_RegExp_sticky, (EjsNativeFunction) sticky, -1, 0);
#else
    ejsBindMethod(ejs, type, ES_RegExp_lastIndex, (EjsNativeFunction) getLastIndex);
    ejsBindMethod(ejs, type, ES_RegExp_set_lastIndex, (EjsNativeFunction) setLastIndex);
    ejsBindMethod(ejs, type, ES_RegExp_global, (EjsNativeFunction) getGlobalFlag);
    ejsBindMethod(ejs, type, ES_RegExp_ignoreCase, (EjsNativeFunction) getIgnoreCase);
    ejsBindMethod(ejs, type, ES_RegExp_multiline, (EjsNativeFunction) getMultiline);
    ejsBindMethod(ejs, type, ES_RegExp_source, (EjsNativeFunction) getSource);
    ejsBindMethod(ejs, type, ES_RegExp_matched, (EjsNativeFunction) matched);
    ejsBindMethod(ejs, type, ES_RegExp_start, (EjsNativeFunction) start);
    ejsBindMethod(ejs, type, ES_RegExp_sticky, (EjsNativeFunction) sticky);
#endif
    ejsBindMethod(ejs, type, ES_RegExp_test, (EjsNativeFunction) test);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) regExpToString);
}

#else
void __dummyRegExp() {}
#endif /* BLD_FEATURE_REGEXP */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsRegExp.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsString.c"
 */
/************************************************************************/

/**
 *  ejsString.c - Ejscript string class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int catString(Ejs *ejs, EjsString *dest, char *str, int len);
static int indexof(cchar *str, int len, cchar *pattern, int patlen, int dir);

/*
 *  Cast the string operand to a primitive type
 */

static EjsVar *castString(Ejs *ejs, EjsString *sp, EjsType *type)
{
    mprAssert(sp);
    mprAssert(type);

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejsParseVar(ejs, sp->value, ES_Number);

#if ES_RegExp && BLD_FEATURE_REGEXP
    case ES_RegExp:
        if (sp->value && sp->value[0] == '/') {
            return (EjsVar*) ejsCreateRegExp(ejs, sp->value);
        } else {
            EjsVar      *result;
            char        *buf;
            mprAllocStrcat(ejs, &buf, -1, NULL, "/", sp->value, "/", NULL);
            result = (EjsVar*) ejsCreateRegExp(ejs, buf);
            mprFree(buf);
            return result;
        }
#endif

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
    return 0;
}


/*
 *  Clone a string. Shallow copies simply return a reference as strings are immutable.
 */
static EjsString *cloneString(Ejs *ejs, EjsString *sp, bool deep)
{
    if (deep) {
        return ejsCreateStringWithLength(ejs, sp->value, sp->length);
    }
    return sp;
}


static void destroyString(Ejs *ejs, EjsString *sp)
{
    mprAssert(sp);

    mprFree(sp->value);
    sp->value = 0;
    ejsFreeVar(ejs, (EjsVar*) sp);
}


/*
 *  Get a string element. Slot numbers correspond to character indicies.
 */
static EjsVar *getStringProperty(Ejs *ejs, EjsString *sp, int index)
{
    if (index < 0 || index >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad string subscript");
        return 0;
    }
    return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[index], 1);
}


static EjsVar *coerceStringOperands(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToString(ejs, rhs));

    /*
     *  Overloaded operators
     */
    case EJS_OP_MUL:
        if (ejsIsNumber(rhs)) {
            return 0;
        }
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    case EJS_OP_REM:
        return 0;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_OR:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToString(ejs, rhs));

    case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToBoolean(ejs, lhs), opcode, rhs);

    case EJS_OP_NOT:
    case EJS_OP_NEG:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) (((EjsString*) lhs)->value ? ejs->trueValue : ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsString*) lhs)->value ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


static EjsVar *invokeStringOperator(Ejs *ejs, EjsString *lhs, int opcode,  EjsString *rhs, void *data)
{
    EjsVar      *result;
#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
    EjsVar      *arg;
#endif

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceStringOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }
    /*
     *  Types now match, both strings
     */
    switch (opcode) {
    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_EQ:
        if (lhs == rhs || (lhs->value == rhs->value)) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) == 0);

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        if (lhs->length != rhs->length) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) != 0);

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) < 0);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) <= 0);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) > 0);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) >= 0);

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);


    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        result = (EjsVar*) ejsCreateString(ejs, lhs->value);
        ejsStrcat(ejs, (EjsString*) result, (EjsVar*) rhs);
        return result;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_OR:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, (EjsVar*) lhs), opcode, (EjsVar*) rhs);

#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
    /*
     *  Overloaded
     */
    case EJS_OP_SUB:
        arg = (EjsVar*) rhs;
        //  TODO OPT - inline this capability
        return ejsRunFunctionBySlot(ejs, (EjsVar*) lhs, ES_String_MINUS, 1, &arg);

    case EJS_OP_REM:
        arg = (EjsVar*) rhs;
        return ejsRunFunctionBySlot(ejs, (EjsVar*) lhs, ES_String_MOD, 1, &arg);
#endif

    case EJS_OP_NEG:
    case EJS_OP_LOGICAL_NOT:
    case EJS_OP_NOT:
        /* Already handled in coerceStringOperands */
    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
    mprAssert(0);
}


/*
 *  Lookup an string index.
 */
static int lookupStringProperty(struct Ejs *ejs, EjsString *sp, EjsName *qname)
{
    int     index;

    if (qname == 0 || ! isdigit((int) qname->name[0])) {
        return EJS_ERR;
    }
    index = atoi(qname->name);
    if (index < sp->length) {
        return index;
    }

    return EJS_ERR;
}


/*
 *  String constructor.
 *
 *      function String()
 *      function String(str: String)
 */
//  TODO - refactor

static EjsVar *stringConstructor(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsString   *str;

    mprAssert(argc == 0 || (argc == 1 && ejsIsArray(argv[0])));

    if (argc == 1) {
        args = (EjsArray*) argv[0];
        if (args->length > 0) {
            str = ejsToString(ejs, ejsGetProperty(ejs, (EjsVar*) args, 0));
            if (str) {
                sp->value = mprStrdup(sp, str->value);
                sp->length = str->length;
            }
        } else {
            sp->value = mprStrdup(ejs, "");
            if (sp->value == 0) {
                return 0;
            }
            sp->length = 0;
        }

    } else {
        sp->value = mprStrdup(ejs, "");
        if (sp->value == 0) {
            return 0;
        }
        sp->length = 0;
    }
    return (EjsVar*) sp;
}


/*
 *  Do a case sensitive comparison between this string and another.
 *
 *  function caseCompare(compare: String): Number
 */
static EjsVar *caseCompare(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     result;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    result = mprStrcmp(sp->value, ((EjsString*) argv[0])->value);

    return (EjsVar*) ejsCreateNumber(ejs, result);
}


/*
 *  Return a string containing the character at a given index
 *
 *  function charAt(index: Number): String
 */
static EjsVar *charAt(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     index;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    index = ejsGetInt(argv[0]);
    if (index < 0 || index >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad string subscript");
        return 0;
    }

    return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[index], 1);
}


/*
 *  Return an integer containing the character at a given index
 *
 *  function charCodeAt(index: Number = 0): Number
 */

static EjsVar *charCodeAt(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     index;

    index = (argc == 1) ? ejsGetInt(argv[0]) : 0;

    if (index < 0 || index >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad string subscript");
        return 0;
    }

    return (EjsVar*) ejsCreateNumber(ejs, sp->value[index]);
}


/*
 *  Catenate args to a string and return a new string.
 *
 *  function concat(...args): String
 */
static EjsVar *concatString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsString   *result;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));
    args = (EjsArray*) argv[0];

    result = ejsDupString(ejs, sp);

    count = ejsGetPropertyCount(ejs, (EjsVar*) args);
    for (i = 0; i < args->length; i++) {
        if (ejsStrcat(ejs, result, ejsGetProperty(ejs, (EjsVar*) args, i)) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }
    return (EjsVar*) result;
}


/**
 *  Check if a string contains the pattern (string or regexp)
 *
 *  function contains(pattern: Object): Boolean
 */
static EjsVar *containsString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsVar      *pat;

    pat = argv[0];

    if (ejsIsString(pat)) {
        return (EjsVar*) ejsCreateBoolean(ejs, strstr(sp->value, ((EjsString*) pat)->value) != 0);

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(pat)) {
        EjsRegExp   *rp;
        int         count;
        rp = (EjsRegExp*) argv[0];
        count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, 0, 0, 0, 0);
        return (EjsVar*) ejsCreateBoolean(ejs, count >= 0);
#endif
    }
    ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
    return 0;
}


/**
 *  Check if a string ends with a given pattern
 *
 *  function endsWith(pattern: String): Boolean
 */
static EjsVar *endsWith(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    char        *pattern;
    int         len;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    pattern = ejsGetString(argv[0]);
    len = (int) strlen(pattern);
    if (len > sp->length) {
        return (EjsVar*) ejs->falseValue;
    }
    return (EjsVar*) ejsCreateBoolean(ejs, strncmp(&sp->value[sp->length - len], pattern, len) == 0);
}


/**
 *  Format the arguments
 *
 *  function format(...args): String
 *
 *  Format:         %[modifier][width][precision][bits][type]
 */
static EjsVar *formatString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *args, *inner;
    EjsString   *result;
    EjsVar      *value;
    char        *buf;
    char        fmt[16];
    int         c, i, len, nextArg, start, kind, last;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    /*
     *  Flatten the args if there is only one element and it is itself an array. This happens when invoked
     *  via the overloaded operator '%' which in turn invokes format()
     */
    if (args->length == 1) {
        inner = (EjsArray*) ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (ejsIsArray(inner)) {
            args = inner;
        }
    }

    result = ejsCreateString(ejs, 0);

    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    /*
     *  Parse the format string and extract one specifier at a time.
     */
    last = 0;
    for (i = 0, nextArg = 0; i < sp->length && nextArg < args->length; i++) {
        c = sp->value[i];
        if (c != '%') {
            continue;
        }

        if (i > last) {
            catString(ejs, result, &sp->value[last], i - last);
        }

        /*
         *  Find the end of the format specifier and determine the format type (kind)
         */
        start = i++;
        i += (int) strspn(&sp->value[i], "-+ #,0*123456789.hlL");
        kind = sp->value[i];

        if (strchr("cdefginopsSuxX", kind)) {
            len = i - start + 1;
            mprMemcpy(fmt, sizeof(fmt), &sp->value[start], len);
            fmt[len] = '\0';

            value = ejsGetProperty(ejs, (EjsVar*) args, nextArg);

            buf = 0;
            switch (kind) {
            case 'd': case 'i': case 'o': case 'u':
                value = (EjsVar*) ejsToNumber(ejs, value);
                len = mprAllocSprintf(ejs, &buf, -1, fmt, (int64) ejsGetNumber(value));
                break;
#if BLD_FEATURE_FLOATING_POINT
            case 'e': case 'g': case 'f':
                value = (EjsVar*) ejsToNumber(ejs, value);
                len = mprAllocSprintf(ejs, &buf, -1, fmt, (double) ejsGetNumber(value));
                break;
#endif
            case 's':
                value = (EjsVar*) ejsToString(ejs, value);
                len = mprAllocSprintf(ejs, &buf, -1, fmt, ejsGetString(value));
                break;

            case 'X': case 'x':
                len = mprAllocSprintf(ejs, &buf, -1, fmt, (int64) ejsGetNumber(value));
                break;

            case 'n':
                len = mprAllocVsprintf(ejs, &buf, -1, fmt, 0);

            default:
                ejsThrowArgError(ejs, "Bad format specifier");
                return 0;
            }
            catString(ejs, result, buf, len);
            mprFree(buf);
            last = i + 1;
            nextArg++;
        }
    }

    i = (int) strlen(sp->value);
    if (i > last) {
        catString(ejs, result, &sp->value[last], i - last);
    }

    return (EjsVar*) result;
}


/*
 *  Create a string from character codes
 *
 *  static function fromCharCode(...codes): String
 */
static EjsVar *fromCharCode(Ejs *ejs, EjsString *unused, int argc, EjsVar **argv)
{
    EjsString   *result;
    EjsArray    *args;
    EjsVar      *vp;
    int         i;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));
    args = (EjsArray*) argv[0];

    result = (EjsString*) ejsCreateBareString(ejs, argc + 1);
    if (result == 0) {
        return 0;
    }

    for (i = 0; i < args->length; i++) {
        vp = ejsGetProperty(ejs, (EjsVar*) args, i);
        result->value[i] = ejsGetInt(ejsToNumber(ejs, vp));
    }
    result->value[i] = '\0';
    result->length = args->length;

    return (EjsVar*) result;
}


/*
 *  Function to iterate and return the next character code.
 *  NOTE: this is not a method of String. Rather, it is a callback function for Iterator
 */
static EjsVar *nextStringKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsString   *sp;

    sp = (EjsString*) ip->target;

    if (!ejsIsString(sp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < sp->length) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator function get(): Iterator
 */
static EjsVar *getStringIterator(Ejs *ejs, EjsVar *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, sp, (EjsNativeFunction) nextStringKey, 0, NULL);
}


/*
 *  Function to iterate and return the next string character (as a string).
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextStringValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsString   *sp;

    sp = (EjsString*) ip->target;
    if (!ejsIsString(sp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < sp->length) {
        return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[ip->index++], 1);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator function getValues(): Iterator
 */
static EjsVar *getStringValues(Ejs *ejs, EjsVar *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, sp, (EjsNativeFunction) nextStringValue, 0, NULL);
}


/*
 *  Get the length of a string.
 *  @return Returns the number of characters in the string
 *
 *  override function get length(): Number
 */

static EjsVar *stringLength(Ejs *ejs, EjsString *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


/*
 *  Return the position of the first occurance of a substring
 *
 *  function indexOf(pattern: String, startIndex: Number = 0): Number
 */
static EjsVar *indexOf(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    char    *pattern;
    int     index, start, patternLength;

    mprAssert(1 <= argc && argc <= 2);
    mprAssert(ejsIsString(argv[0]));

    pattern = ejsGetString(argv[0]);
    patternLength = ((EjsString*) argv[0])->length;

    if (argc == 2) {
        start = ejsGetInt(argv[1]);
        if (start > sp->length) {
            start = sp->length;
        }
    } else {
        start = 0;
    }

    index = indexof(&sp->value[start], sp->length - start, pattern, patternLength, 1);
    if (index < 0) {
        return (EjsVar*) ejs->minusOneValue;
    }
    return (EjsVar*) ejsCreateNumber(ejs, index + start);
}


static EjsVar *isAlpha(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isalpha((int) sp->value[0]));
}


static EjsVar *isDigit(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isdigit((int) sp->value[0]));
}


static EjsVar *isLower(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, islower((int) sp->value[0]));
}


static EjsVar *isSpace(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isspace((int) sp->value[0]));
}


static EjsVar *isUpper(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isupper((int) sp->value[0]));
}


/*
 *  Return the position of the last occurance of a substring
 *
 *  function lastIndexOf(pattern: String, start: Number = -1): Number
 */
static EjsVar *lastIndexOf(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    char    *pattern;
    int     start, patternLength, index;

    mprAssert(1 <= argc && argc <= 2);

    pattern = ejsGetString(argv[0]);
    patternLength = ((EjsString*) argv[0])->length;

    if (argc == 2) {
        start = ejsGetInt(argv[1]);
        if (start > sp->length) {
            start = sp->length;
        }

    } else {
        start = 0;
    }

    if (start < 0 || start >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad start subscript");
        return 0;
    }

    index = indexof(sp->value, sp->length, pattern, patternLength, -1);
    if (index < 0) {
        return (EjsVar*) ejs->minusOneValue;
    }
    return (EjsVar*) ejsCreateNumber(ejs, index);
}


#if BLD_FEATURE_REGEXP
/*
 *  Match a pattern
 *
 *  function match(pattern: RegExp): Array
 */
static EjsVar *match(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsRegExp   *rp;
    EjsArray    *results;
    EjsString   *match;
    int         matches[EJS_MAX_REGEX_MATCHES * 3];
    int         count, len, resultCount;

    rp = (EjsRegExp*) argv[0];
    rp->endLastMatch = 0;
    results = NULL;
    resultCount = 0;

    do {
        count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, rp->endLastMatch, 0, matches, sizeof(matches) / sizeof(int));
        if (count <= 0) {
            break;
        }
        if (results == 0) {
            results = ejsCreateArray(ejs, count);
        }
        len = matches[1] - matches[0];
        match = ejsCreateStringWithLength(ejs, &sp->value[matches[0]], len);
        ejsSetProperty(ejs, (EjsVar*) results, resultCount++, (EjsVar*) match);
        rp->endLastMatch = matches[1];

    } while (rp->global);

    if (results == NULL) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) results;
}
#endif


#if ES_String_parseJSON
static EjsVar *parseJSON(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    return 0;
}
#endif


static EjsVar *printable(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;
    char            buf[8];
    int             i, j, k, len, nonprint;

    nonprint = 0;
    for (i = 0; i < sp->length; i++)  {
        if (!isprint((int) sp->value[i])) {
            nonprint++;
        }
    }
    if (nonprint == 0) {
        return (EjsVar*) sp;
    }

    result = ejsCreateBareString(ejs, sp->length + (nonprint * 5));
    if (result == 0) {
        return 0;
    }
    for (i = 0, j = 0; i < sp->length; i++)  {
        if (isprint((int) sp->value[i])) {
            result->value[j++] = sp->value[i];
        } else {
            result->value[j++] = '\\';
            result->value[j++] = 'u';
            mprItoa(buf, 4, sp->value[i], 16);
            len = (int) strlen(buf);
            for (k = len; k < 4; k++) {
                result->value[j++] = '0';
            }
            for (k = 0; buf[k]; k++) {
                result->value[j++] = buf[k];
            }
        }
    }
    result->value[j] = '\0';
    return (EjsVar*) result;
}


static EjsVar *quote(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;

    result = ejsCreateBareString(ejs, sp->length + 2);
    if (result == 0) {
        return 0;
    }

    memcpy(&result->value[1], sp->value, sp->length);
    result->value[0] = '"';
    result->value[sp->length + 1] = '"';
    result->value[sp->length + 2] = '\0';
    result->length = sp->length + 2;

    return (EjsVar*) result;
}


/*
 *  Remove characters and return a new string.
 *
 *  function remove(start: Number, end: Number = -1): String
 *
 */
static EjsVar *removeCharsFromString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;
    int             start, end, i, j;

    mprAssert(1 <= argc && argc <= 2);

    start = ejsGetInt(argv[0]);
    end = ejsGetInt(argv[1]);

    if (start < 0) {
        start += sp->length;
    }
    if (end < 0) {
        end += sp->length;
    }
    if (start >= sp->length) {
        start = sp->length - 1;
    }
    if (end > sp->length) {
        end = sp->length;
    }

    result = ejsCreateBareString(ejs, sp->length - (end - start));
    if (result == 0) {
        return 0;
    }
    for (j = i = 0; i < start; i++, j++) {
        result->value[j] = sp->value[i];
    }
    for (i = end; i < sp->length; i++, j++) {
        result->value[j] = sp->value[i];
    }
    result->value[j] = '\0';

    return (EjsVar*) result;
}


/*
 *  Search and replace.
 *
 *  function replace(pattern: (String|Regexp), replacement: String): String
 *
 */
static EjsVar *replace(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString   *result, *replacement;
    char        *pattern;
    int         index, patternLength;

    mprAssert(argc == 2);
    result = 0;
    replacement = (EjsString*) argv[1];

    if (ejsIsString(argv[0])) {
        pattern = ejsGetString(argv[0]);
        patternLength = ((EjsString*) argv[0])->length;

        index = indexof(sp->value, sp->length, pattern, patternLength, 1);
        if (index >= 0) {
            result = ejsCreateString(ejs, 0);
            if (result == 0) {
                return 0;
            }
            catString(ejs, result, sp->value, index);
            catString(ejs, result, replacement->value, replacement->length);

            index += patternLength;
            if (index < sp->length) {
                catString(ejs, result, &sp->value[index], sp->length - index);
            }

        } else {
            result = ejsDupString(ejs, sp);
        }

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(argv[0])) {
        EjsRegExp   *rp;
        char        *cp, *lastReplace, *end;
        int         matches[EJS_MAX_REGEX_MATCHES * 3];
        int         count, endLastMatch, submatch;

        rp = (EjsRegExp*) argv[0];
        result = ejsCreateString(ejs, 0);
        endLastMatch = 0;

        do {
            count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, endLastMatch, 0, matches, sizeof(matches) / sizeof(int));
            if (count <= 0) {
                break;
            }

            if (endLastMatch < matches[0]) {
                /* Append prior string text */
                catString(ejs, result, &sp->value[endLastMatch], matches[0] - endLastMatch);
            }

            /*
             *  Process the replacement template
             */
            end = &replacement->value[replacement->length];
            lastReplace = replacement->value;

            for (cp = replacement->value; cp < end; ) {
                if (*cp == '$') {
                    if (lastReplace < cp) {
                        catString(ejs, result, lastReplace, (int) (cp - lastReplace));
                    }
                    switch (*++cp) {
                    case '$':
                        catString(ejs, result, "$", 1);
                        break;
                    case '&':
                        /* Replace the matched string */
                        catString(ejs, result, &sp->value[matches[0]], matches[1] - matches[0]);
                        break;
                    case '`':
                        /* Insert the portion that preceeds the matched string */
                        catString(ejs, result, sp->value, matches[0]);
                        break;
                    case '\'':
                        /* Insert the portion that follows the matched string */
                        catString(ejs, result, &sp->value[matches[1]], sp->length - matches[1]);
                        break;
                    default:
                        /* Insert the nth submatch */
                        if (isdigit((int) *cp)) {
                            submatch = mprAtoi(cp, 10);
                            while (isdigit((int) *++cp))
                                ;
                            cp--;
                            if (submatch < count) {
                                submatch *= 2;
                                catString(ejs, result, &sp->value[matches[submatch]], 
                                    matches[submatch + 1] - matches[submatch]);
                            }

                        } else {
                            ejsThrowArgError(ejs, "Bad replacement $ specification");
                            return 0;
                        }
                    }
                    lastReplace = cp + 1;
                }
                cp++;
            }
            if (lastReplace < cp && lastReplace < end) {
                catString(ejs, result, lastReplace, (int) (cp - lastReplace));
            }
            endLastMatch = matches[1];

        } while (rp->global);

        if (endLastMatch < sp->length) {
            /* Append remaining string text */
            catString(ejs, result, &sp->value[endLastMatch], sp->length - endLastMatch);
        }
#endif

    } else {
        ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
        return 0;
    }
    return (EjsVar*) result;
}


static EjsVar *reverseString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int         i, j, tmp;

    if (sp->length <= 1) {
        return (EjsVar*) sp;
    }
    i = (sp->length - 2) / 2;
    j = (sp->length + 1) / 2;
    for (; i >= 0; i--, j++) {
        tmp = sp->value[i];
        sp->value[i] = sp->value[j];
        sp->value[j] = tmp;
    }
    return (EjsVar*) sp;
}


/*
 *  Search for a pattern
 *
 *  function search(pattern: (String | RegExp)): Number
 *
 */
static EjsVar *searchString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    char        *pattern;
    int         index, patternLength;

    if (ejsIsString(argv[0])) {
        pattern = ejsGetString(argv[0]);
        patternLength = ((EjsString*) argv[0])->length;

        index = indexof(sp->value, sp->length, pattern, patternLength, 1);
        return (EjsVar*) ejsCreateNumber(ejs, index);

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(argv[0])) {
        EjsRegExp   *rp;
        int         matches[EJS_MAX_REGEX_MATCHES * 3];
        int         count;
        rp = (EjsRegExp*) argv[0];
        count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, 0, 0, matches, sizeof(matches) / sizeof(int));
        if (count < 0) {
            return (EjsVar*) ejs->minusOneValue;
        }
        return (EjsVar*) ejsCreateNumber(ejs, matches[0]);
#endif

    } else {
        ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
        return 0;
    }
}


/*
 *  Return a substring. End is one past the last character.
 *
 *  function slice(start: Number, end: Number = -1, step: Number = 1): String
 */
static EjsVar *sliceString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;
    int             start, end, step, i, j;

    mprAssert(1 <= argc && argc <= 2);

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        end = ejsGetInt(argv[1]);
    } else {
        end = sp->length;
    }
    if (argc == 3) {
        step = ejsGetInt(argv[1]);
    } else {
        step = 1;
    }

    if (start < 0) {
        start += sp->length;
    }
    if (end < 0) {
        end += sp->length;
    }
    if (step == 0) {
        step = 1;
    }
    if (start < 0 || start >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad start subscript");
        return 0;
    }
    if (end < 0 || end > sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad end subscript");
        return 0;
    }

    result = ejsCreateBareString(ejs, (end - start) / abs(step));
    if (result == 0) {
        return 0;
    }
    if (step > 0) {
        for (i = start, j = 0; i < end; i += step) {
            result->value[j++] = sp->value[i];
        }

    } else {
        for (i = end - 1, j = 0; i >= start; i += step) {
            result->value[j++] = sp->value[i];
        }
    }

    result->value[j] = '\0';
    result->length = j;

    return (EjsVar*) result;
}


/*
 *  Split a string
 *
 *  function split(delimiter: (String | RegExp), limit: Number = -1): Array
 */
static EjsVar *split(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *results;
    EjsString   *elt;
    char        *delim, *cp, *endp;
    int         delimLen, limit;

    mprAssert(1 <= argc && argc <= 2);

    limit = (argc == 2) ? ejsGetInt(argv[1]): -1;
    results = ejsCreateArray(ejs, 0);

    if (ejsIsString(argv[0])) {
        delim = ejsGetString(argv[0]);
        delimLen = (int) strlen(delim);

        endp = cp = sp->value;
        for (; *endp; endp++) {
            if (strncmp(endp, delim, delimLen) == 0 && endp > cp) {
                if (--limit == -1) {
                    return (EjsVar*) results;
                }
                elt = ejsCreateStringWithLength(ejs, cp, (int) (endp - cp));
                ejsSetProperty(ejs, (EjsVar*) results, -1, (EjsVar*) elt);
                cp = endp + delimLen;
                endp = cp;
            }
        }
        if (endp > cp) {
            elt = ejsCreateStringWithLength(ejs, cp, (int) (endp - cp));
            ejsSetProperty(ejs, (EjsVar*) results, -1, (EjsVar*) elt);
        }
        return (EjsVar*) results;

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(argv[0])) {
        EjsRegExp   *rp;
        EjsString   *match;
        int         matches[EJS_MAX_REGEX_MATCHES * 3], count, resultCount;
        rp = (EjsRegExp*) argv[0];
        rp->endLastMatch = 0;
        resultCount = 0;
        do {
            count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, rp->endLastMatch, 0, matches, 
                sizeof(matches) / sizeof(int));
            if (count <= 0) {
                break;
            }
            if (rp->endLastMatch < matches[0]) {
                match = ejsCreateStringWithLength(ejs, &sp->value[rp->endLastMatch], matches[0] - rp->endLastMatch);
                ejsSetProperty(ejs, (EjsVar*) results, resultCount++, (EjsVar*) match);
            }
            rp->endLastMatch = matches[1];
        } while (rp->global);

        if (rp->endLastMatch < sp->length) {
            match = ejsCreateStringWithLength(ejs, &sp->value[rp->endLastMatch], sp->length - rp->endLastMatch);
            ejsSetProperty(ejs, (EjsVar*) results, resultCount++, (EjsVar*) match);
        }
        return (EjsVar*) results;
#endif
    }

    ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
    return 0;
}


/**
 *  Check if a string starts with a given pattern
 *
 *  function startsWith(pattern: String): Boolean
 */
static EjsVar *startsWith(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    char        *pattern;
    int         len;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    pattern = ejsGetString(argv[0]);
    len = (int) strlen(pattern);

    return (EjsVar*) ejsCreateBoolean(ejs, strncmp(&sp->value[0], pattern, len) == 0);
}


#if ES_String_substr
/*
 *  Extract a substring using a length
 *
 *  function substr(start: Number, length: Number = -1): String
 */
static EjsVar *substr(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     start, length;

    length = -1;

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        length = ejsGetInt(argv[1]);
    }

    if (start < 0) {
        start += sp->length - start;
    }
    if (start < 0 || start >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad start subscript");
        return 0;
    }
    if (length < 0 || length > sp->length) {
        length = sp->length - start;
    }


    return (EjsVar*) ejsCreateBinaryString(ejs, &sp->value[start], length);
}
#endif


/*
 *  Extract a substring. Simple routine with positive indicies.
 *
 *  function substring(start: Number, end: Number = -1): String
 */
static EjsVar *substring(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     start, end, tmp;

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        end = ejsGetInt(argv[1]);
    } else {
        end = sp->length;
    }

    if (start < 0) {
        start = 0;
    }
    if (start >= sp->length) {
        start = sp->length - 1;
    }
    if (end < 0) {
        end = sp->length;
    }
    if (end > sp->length) {
        end = sp->length;
    }

    /*
     *  Swap if start is bigger than end
     */
    if (start > end) {
        tmp = start;
        start = end;
        end = tmp;
    }

    return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[start], end - start);
}


/*
 *  Convert the string to camelCase. Return a new string.
 *
 *  function toCamel(): String
 */
static EjsVar *toCamel(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString   *result;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    result->value[0] = tolower((int) sp->value[0]);

    return (EjsVar*) result;
}


#if ES_String_toJSONString
static EjsVar *toJSONString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    return 0;
}
#endif


/*
 *  Convert the string to PascalCase. Return a new string.
 *
 *  function toPascal(): String
 */
static EjsVar *toPascal(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString   *result;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    result->value[0] = toupper((int) sp->value[0]);

    return (EjsVar*) result;
}


/*
 *  Convert the string to lower case.
 *  @return Returns a new lower case version of the string.
 *  @spec ejs-11
 *
 *  function toLower(locale: String = null): String
 *  TODO - locale not supported yet.
 */
static EjsVar *toLower(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    EjsString       *result;
    int             i;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    for (i = 0; i < result->length; i++) {
        result->value[i] = tolower((int) result->value[i]);
    }
    return (EjsVar*) result;
}


/*
 *  Convert to a string
 *
 *  override function toString(): String
 */
static EjsVar *stringToString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) sp;
}


/*
 *  Convert the string to upper case.
 *  @return Returns a new upper case version of the string.
 *  @spec ejs-11
 *
 *  function toUpper(locale: String = null): String
 *  TODO - locale not supported yet.
 */
static EjsVar *toUpper(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    EjsString       *result;
    int             i;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    for (i = 0; i < result->length; i++) {
        result->value[i] = toupper((int) result->value[i]);
    }
    return (EjsVar*) result;
}


/*
 *  Scan the input and tokenize according to the format string
 *
 *  function tokenize(format: String): Array
 */
static EjsVar *tokenize(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *result;
    cchar       *fmt;
    char        *end, *buf;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    buf = sp->value;
    fmt = ejsGetString(argv[0]);
    result = ejsCreateArray(ejs, 0);

    for (fmt = ejsGetString(argv[0]); *fmt && buf < &sp->value[sp->length]; ) {
        if (*fmt++ != '%') {
            continue;
        }
        //  TODO - what other types should we support
        switch (*fmt) {
        case 's':
            for (end = buf; *end; end++) {
                if (isspace((int) *end)) {
                    break;
                }
            }
            ejsSetProperty(ejs, (EjsVar*) result, -1, (EjsVar*) ejsCreateStringWithLength(ejs, buf, (int) (end - buf)));
            buf = end;
            break;

        case 'd':
            ejsSetProperty(ejs, (EjsVar*) result, -1, ejsParseVar(ejs, buf, ES_Number));
            while (*buf && !isspace((int) *buf)) {
                buf++;
            }
            break;

        default:
            ejsThrowArgError(ejs, "Bad format specifier");
            return 0;
        }
        while (*buf && isspace((int) *buf)) {
            buf++;
        }
    }
    return (EjsVar*) result;
}


/**
 *  Returns a trimmed copy of the string. Normally used to trim white space, but can be used to trim any substring
 *  from the start or end of the string.
 *  @param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
 *  @return Returns a (possibly) modified copy of the string
 *  @spec ecma-4
 *
 *  function trim(str: String = null): String
 */
static EjsVar *trimString(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    cchar           *start, *end, *pattern, *mark;
    int             index, patternLength;

    mprAssert(argc == 0 || (argc == 1 && ejsIsString(argv[0])));

    if (argc == 0) {
        for (start = sp->value; start < &sp->value[sp->length]; start++) {
            if (!isspace((int) *start)) {
                break;
            }
        }
        for (end = &sp->value[sp->length - 1]; end >= start; end--) {
            if (!isspace((int) *end)) {
                break;
            }
        }
        end++;

    } else {
        pattern = ejsGetString(argv[0]);
        patternLength = ((EjsString*) argv[0])->length;
        if (patternLength <= 0 || patternLength > sp->length) {
            return (EjsVar*) sp;
        }

        /*
         *  Trim the front
         */
        for (mark = sp->value; &mark[patternLength] < &sp->value[sp->length]; mark += patternLength) {
            index = indexof(mark, patternLength, pattern, patternLength, 1);
            if (index != 0) {
                break;
            }
        }
        start = mark;

        /*
         *  Trim the back
         */
        for (mark = &sp->value[sp->length - patternLength]; mark >= sp->value; mark -= patternLength) {
            index = indexof(mark, patternLength, pattern, patternLength, 1);
            if (index != 0) {
                break;
            }
        }
        end = mark + patternLength;
    }

    return (EjsVar*) ejsCreateStringWithLength(ejs, start, (int) (end - start));
}


/**
 *  Fast append a string. This modifies the original "dest" string. BEWARE: strings are meant to be immutable.
 *  Only use this when constructing strings.
 */
static int catString(Ejs *ejs, EjsString *dest, char *str, int len)
{
    EjsString   *castSrc;
    char        *oldBuf, *buf;
    int         oldLen, newLen;

    mprAssert(dest);

    castSrc = 0;

    oldBuf = dest->value;
    oldLen = dest->length;
    newLen = oldLen + len + 1;

#if FUTURE
    if (newLen < MPR_SLAB_STR_MAX) {
        buf = oldBuf;
    } else {
#endif
        buf = (char*) mprRealloc(ejs, oldBuf, newLen);
        if (buf == 0) {
            return -1;
        }
        dest->value = buf;
#if FUTURE
    }
#endif
    memcpy(&buf[oldLen], str, len);
    dest->length += len;
    buf[dest->length] = '\0';

    return 0;
}


/**
 *  Fast append a string. This modifies the original "dest" string. BEWARE: strings are meant to be immutable.
 *  Only use this when constructing strings.
 */
int ejsStrcat(Ejs *ejs, EjsString *dest, EjsVar *src)
{
    EjsString   *castSrc;
    char        *str;
    int         len;

    mprAssert(dest);

    castSrc = 0;

    if (ejsIsString(dest)) {
        if (! ejsIsString(src)) {
            castSrc = (EjsString*) ejsToString(ejs, src);
            if (castSrc == 0) {
                return -1;
            }
            len = (int) strlen(castSrc->value);
            str = castSrc->value;

        } else {
            str = ((EjsString*) src)->value;
            len = ((EjsString*) src)->length;
        }

        if (catString(ejs, dest, str, len) < 0) {
            return -1;
        }

    } else {
        /*
         *  Convert the source to a string and then steal the rusult buffer and assign to the destination
         *  TODO - should be freeing the destination string.
         */
        castSrc = (EjsString*) ejsToString(ejs, src);
        dest->value = castSrc->value;
        mprStealBlock(dest, dest->value);
        castSrc->value = 0;
    }

    return 0;
}


/*
 *  Copy a string. Always null terminate.
 */
int ejsStrdup(MprCtx ctx, uchar **dest, cvoid *src, int nbytes)
{
    mprAssert(dest);
    mprAssert(src);

    if (nbytes > 0) {
        *dest = (uchar*) mprAlloc(ctx, nbytes + 1);
        if (*dest == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        strncpy((char*) *dest, (char*) src, nbytes);

    } else {
        *dest = (uchar*) mprAlloc(ctx, 1);
        nbytes = 0;
    }

    (*dest)[nbytes] = '\0';

    return nbytes;
}


/*
 *  Find a substring. Search forward or backwards. Return the index in the string where the pattern was found.
 *  Return -1 if not found.
 */
static int indexof(cchar *str, int len, cchar *pattern, int patternLength, int dir)
{
    cchar   *s1, *s2;
    int     i, j;

    mprAssert(dir == 1 || dir == -1);

    if (dir > 0) {
        for (i = 0; i < len; i++) {
            s1 = &str[i];
            for (j = 0, s2 = pattern; j < patternLength; s1++, s2++, j++) {
                if (*s1 != *s2) {
                    break;
                }
            }
            if (*s2 == '\0') {
                return i;
            }
        }

    } else {
        for (i = len - 1; i >= 0; i--) {
            s1 = &str[i];
            for (j = 0, s2 = pattern; j < patternLength; s1++, s2++, j++) {
                if (*s1 != *s2) {
                    break;
                }
            }
            if (*s2 == '\0') {
                return i;
            }
        }
    }
    return -1;
}



EjsString *ejsCreateString(Ejs *ejs, cchar *value)
{
    EjsString   *sp;

    /*
     *  No need to invoke constructor
     */
    sp = (EjsString*) ejsCreateVar(ejs, ejs->stringType, 0);
    if (sp != 0) {
        sp->value = mprStrdup(ejs, value);
        if (sp->value == 0) {
            return 0;
        }
        sp->length = (int) strlen(sp->value);

        ejsSetDebugName(sp, sp->value);
    }
    return sp;
}


EjsString *ejsDupString(Ejs *ejs, EjsString *sp)
{
    return ejsCreateStringWithLength(ejs, sp->value, sp->length);
}


/*
 *  Initialize a binary string value.
 */
EjsString *ejsCreateStringWithLength(Ejs *ejs, cchar *value, int len)
{
    EjsString   *sp;
    uchar       *dest;

    //  TODO - OPT Would be much faster to allocate the string value in the actual object since strings are 
    //      immutable
    sp = (EjsString*) ejsCreateVar(ejs, ejs->stringType, 0);
    if (sp != 0) {
        sp->length = ejsStrdup(ejs, &dest, value, len);
        sp->value = (char*) dest;
        if (sp->length < 0) {
            return 0;
        }
    }
    return sp;
}


/*
 *  Initialize an string with a pre-allocated buffer but without data..
 */
EjsString *ejsCreateBareString(Ejs *ejs, int len)
{
    EjsString   *sp;

    //  TODO - OPT Would be much faster to allocate the string value in the actual object since strings are immutable
    sp = (EjsString*) ejsCreateVar(ejs, ejs->stringType, 0);
    if (sp != 0) {
        sp->value = mprAlloc(sp, len + 1);
        if (sp->value == 0) {
            return 0;
        }
        sp->length = len;
        sp->value[len] = '\0';
    }
    return sp;
}


void ejsCreateStringType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "String"), ejs->objectType, sizeof(EjsString),
        ES_String, ES_String_NUM_CLASS_PROP,  ES_String_NUM_INSTANCE_PROP,
        EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OPER_OVERLOAD);
    ejs->stringType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castString;
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneString;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyString;
    type->helpers->getProperty = (EjsGetPropertyHelper) getStringProperty;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeStringOperator;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupStringProperty;

    type->numericIndicies = 1;

    /*
     *  Pre-create the empty string.
     */
    ejs->emptyStringValue = (EjsString*) ejsCreateString(ejs, "");
    ejsSetDebugName(ejs->emptyStringValue, "emptyString");
}


void ejsConfigureStringType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->stringType;

    /*
     *  Define the "string" alias
     */
    ejsSetProperty(ejs, ejs->global, ES_string, (EjsVar*) type);
    ejsBindMethod(ejs, type, ES_String_String, (EjsNativeFunction) stringConstructor);
    ejsBindMethod(ejs, type, ES_String_caseCompare, (EjsNativeFunction) caseCompare);
    ejsBindMethod(ejs, type, ES_String_charAt, (EjsNativeFunction) charAt);
    ejsBindMethod(ejs, type, ES_String_charCodeAt, (EjsNativeFunction) charCodeAt);
    ejsBindMethod(ejs, type, ES_String_concat, (EjsNativeFunction) concatString);
    ejsBindMethod(ejs, type, ES_String_contains, (EjsNativeFunction) containsString);
    ejsBindMethod(ejs, type, ES_String_endsWith, (EjsNativeFunction) endsWith);
    ejsBindMethod(ejs, type, ES_String_format, (EjsNativeFunction) formatString);
    ejsBindMethod(ejs, type, ES_String_fromCharCode, (EjsNativeFunction) fromCharCode);
    ejsBindMethod(ejs, type, ES_Object_get, (EjsNativeFunction) getStringIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, (EjsNativeFunction) getStringValues);
    ejsBindMethod(ejs, type, ES_String_indexOf, (EjsNativeFunction) indexOf);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_String_isDigit, (EjsNativeFunction) isDigit, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isAlpha, (EjsNativeFunction) isAlpha, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isLower, (EjsNativeFunction) isLower, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isSpace, (EjsNativeFunction) isSpace, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isUpper, (EjsNativeFunction) isUpper, -1, 0);
    ejsSetAccessors(ejs, type, ES_Object_length, (EjsNativeFunction) stringLength, -1, 0);
#else
    ejsBindMethod(ejs, type, ES_String_isDigit, (EjsNativeFunction) isDigit);
    ejsBindMethod(ejs, type, ES_String_isAlpha, (EjsNativeFunction) isAlpha);
    ejsBindMethod(ejs, type, ES_String_isLower, (EjsNativeFunction) isLower);
    ejsBindMethod(ejs, type, ES_String_isSpace, (EjsNativeFunction) isSpace);
    ejsBindMethod(ejs, type, ES_String_isUpper, (EjsNativeFunction) isUpper);
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) stringLength);
#endif
    ejsBindMethod(ejs, type, ES_String_lastIndexOf, (EjsNativeFunction) lastIndexOf);
#if BLD_FEATURE_REGEXP
    ejsBindMethod(ejs, type, ES_String_match, (EjsNativeFunction) match);
#endif
    ejsBindMethod(ejs, type, ES_String_remove, (EjsNativeFunction) removeCharsFromString);
    ejsBindMethod(ejs, type, ES_String_slice, (EjsNativeFunction) sliceString);
    ejsBindMethod(ejs, type, ES_String_split, (EjsNativeFunction) split);
#if ES_String_localeCompare && FUTURE
    ejsBindMethod(ejs, type, ES_String_localeCompare, (EjsNativeFunction) localeCompare);
#endif
#if ES_String_parseJSON
    ejsBindMethod(ejs, type, ES_String_parseJSON, (EjsNativeFunction) parseJSON);
#endif
    ejsBindMethod(ejs, type, ES_String_printable, (EjsNativeFunction) printable);
    ejsBindMethod(ejs, type, ES_String_quote, (EjsNativeFunction) quote);
    ejsBindMethod(ejs, type, ES_String_replace, (EjsNativeFunction) replace);
    ejsBindMethod(ejs, type, ES_String_reverse, (EjsNativeFunction) reverseString);
    ejsBindMethod(ejs, type, ES_String_search, (EjsNativeFunction) searchString);
    ejsBindMethod(ejs, type, ES_String_startsWith, (EjsNativeFunction) startsWith);
    ejsBindMethod(ejs, type, ES_String_substring, (EjsNativeFunction) substring);
#if ES_String_substr
    ejsBindMethod(ejs, type, ES_String_substr, (EjsNativeFunction) substr);
#endif
    ejsBindMethod(ejs, type, ES_String_toCamel, (EjsNativeFunction) toCamel);
    ejsBindMethod(ejs, type, ES_String_toLower, (EjsNativeFunction) toLower);
    ejsBindMethod(ejs, type, ES_String_toPascal, (EjsNativeFunction) toPascal);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) stringToString);
    ejsBindMethod(ejs, type, ES_String_toUpper, (EjsNativeFunction) toUpper);
    ejsBindMethod(ejs, type, ES_String_tokenize, (EjsNativeFunction) tokenize);
    ejsBindMethod(ejs, type, ES_String_trim, (EjsNativeFunction) trimString);

#if FUTURE
    ejsBindMethod(ejs, type, ES_String_LBRACKET, operLBRACKET);
    ejsBindMethod(ejs, type, ES_String_PLUS, operPLUS);
    ejsBindMethod(ejs, type, ES_String_MINUS, operMINUS);
    ejsBindMethod(ejs, type, ES_String_LT, operLT);
    ejsBindMethod(ejs, type, ES_String_GT, operGT);
    ejsBindMethod(ejs, type, ES_String_EQ, operEQ);
    ejsBindMethod(ejs, type, ES_String_MOD, operMOD);
    ejsBindMethod(ejs, type, ES_String_MUL, operMUL);
#endif
}

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsString.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsType.c"
 */
/************************************************************************/

/**
 *  ejsType.c - Type class
 *
 *  The type class is the base class for all types (classes) in the system.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static EjsType *createBootstrapType(Ejs *ejs, int numSlots, int attributes);
static EjsType *createType(Ejs *ejs, EjsName *qname, EjsModule *up, EjsType *baseType, int instanceSize, int numSlots, 
        int attributes, void *typeData);
static EjsBlock *createInstanceBlock(Ejs *ejs, cchar *name, EjsBlock *baseBlock, int numSlots, int attributes);

/*
 *  Copy a type. 
 *
 *  intrinsic function copy(type: Object): Object
 */

static EjsType *cloneTypeVar(Ejs *ejs, EjsType *src, bool deep)
{
    EjsType     *dest;

    if (! ejsIsType(src)) {
        ejsThrowTypeError(ejs, "Expecting a Type object");
        return 0;
    }

    dest = (EjsType*) (ejs->blockHelpers->cloneVar)(ejs, (EjsVar*) src, deep);
    if (dest == 0) {
        return dest;
    }

    dest->qname = src->qname;
    dest->baseType = src->baseType;
    dest->instanceBlock = src->instanceBlock;
    dest->instanceSize = src->instanceSize;
    dest->helpers = src->helpers;
    dest->module = src->module;
    dest->typeData = src->typeData;
    dest->id = src->id;

    /*
     *  TODO OPT bitfields
     */
    dest->subTypeCount = src->subTypeCount;
    dest->numAlloc = src->numAlloc;
    dest->callsSuper = src->callsSuper;
    dest->dynamicInstance = src->dynamicInstance;
    dest->separateInstanceSlots = src->separateInstanceSlots;
    dest->final = src->final;
    dest->fixupDone = src->fixupDone;
    dest->hasBaseConstructors = src->hasBaseConstructors;
    dest->hasBaseInitializers = src->hasBaseInitializers;
    dest->hasBaseStaticInitializers = src->hasBaseStaticInitializers;
    dest->hasConstructor = src->hasConstructor;
    dest->hasFinalizer = src->hasFinalizer;
    dest->hasInitializer = src->hasInitializer;
    dest->hasObject = src->hasObject;
    dest->hasStaticInitializer = src->hasStaticInitializer;
    dest->initialized = src->initialized;
    dest->isInterface = src->isInterface;
    dest->operatorOverload = src->operatorOverload;
    dest->needFixup = src->needFixup;
    dest->nobind = src->nobind;
    dest->numericIndicies = src->numericIndicies;
    dest->skipScope = src->skipScope;

    /* Don't copy pool. The cloned copy will have its own pool */

    return dest;
}


/*
 *  Create a new uninitialized "type" object. numSlots is the number of property slots to pre-allocate.
 */
static EjsType *createTypeVar(Ejs *ejs, EjsType *typeType, int numSlots, int attributes)
{
    EjsType         *type;
    EjsObject       *obj;
    EjsHashEntry    *entries;
    char            *start;
    int             hashSize, dynamic, i;

    mprAssert(ejs);

    hashSize = 0;

    /*
     *  If the compiler is building itself (empty mode), then the types themselves must be dynamic. Otherwise, the type
     *  is fixed and will contain the names hash and traits in one memory block.
     */
    if ((ejs->flags & (EJS_FLAG_EMPTY | EJS_FLAG_COMPILER))) {
        typeType->instanceSize = sizeof(EjsType);
        dynamic = 1;

    } else {
        typeType->instanceSize = sizeof(EjsType);
        typeType->instanceSize += (int) sizeof(EjsTrait) * numSlots;
        typeType->instanceSize += (int) sizeof(EjsNames) + ((int) sizeof(EjsHashEntry) * numSlots);
        dynamic = 0;

        if (numSlots > EJS_HASH_MIN_PROP) {
            hashSize = ejsGetHashSize(numSlots);
            typeType->instanceSize += (hashSize * (int) sizeof(int));
        }
    }

    /*
     *  NOTE: all types will be created in the eternal generation (see ejsAllocVar in ejsGarbage.c)
     */
    type = (EjsType*) ejsCreateObjectEx(ejs, typeType, numSlots, dynamic);
    if (type == 0) {
        return 0;
    }

    type->numAlloc = 1;
    type->block.obj.var.isType = 1;
    type->subTypeCount = typeType->subTypeCount + 1;
    type->block.obj.var.type = typeType;
    type->block.obj.var.dynamic = dynamic;

    ejsInitList(&type->block.namespaces);

    if (dynamic) {
        if (ejsGrowBlock(ejs, (EjsBlock*) type, numSlots) < 0) {
            return 0;
        }
        type->block.numTraits = numSlots;
        mprAssert(type->block.numTraits <= type->block.sizeTraits);
                      
    } else {
        obj = (EjsObject*) type;
        start = (char*) type + sizeof(EjsType);

        obj->names = (EjsNames*) start;
        start += sizeof(EjsNames);

        entries = obj->names->entries = (EjsHashEntry*) start;
        start += (sizeof(EjsHashEntry) * numSlots);
        obj->names->sizeEntries = numSlots;

        for (i = 0; i < numSlots; i++) {
            entries[i].nextSlot = -1;
            entries[i].qname.name = "";
            entries[i].qname.space = "";
        }
        
        type->block.traits = (EjsTrait*) start;
        start += sizeof(EjsTrait) * numSlots;
        type->block.sizeTraits = numSlots;
        type->block.numTraits = numSlots;
        
        if (hashSize > 0) {
            obj->names->buckets = (int*) start;
            start += sizeof(int) * hashSize;
            obj->names->sizeBuckets = hashSize;
            memset(obj->names->buckets, -1, hashSize * sizeof(int));
        }
        
        mprAssert((start - (char*) type) <= typeType->instanceSize);

        typeType->instanceSize = sizeof(EjsType);
    }

    return type;
}


/*
 *  Create a bootstrap type variable. This is used for the Object, Block and Type types.
 */
static EjsType *createBootstrapType(Ejs *ejs, int numSlots, int attributes)
{
    EjsType     *type, bootstrap;
    EjsBlock    bootstrapInstanceBlock;

    mprAssert(ejs);

    memset(&bootstrap, 0, sizeof(bootstrap));
    memset(&bootstrapInstanceBlock, 0, sizeof(bootstrapInstanceBlock));

    bootstrap.instanceSize = sizeof(EjsType);
    bootstrap.subTypeCount = -1;
    bootstrap.instanceBlock = &bootstrapInstanceBlock;

    type = (EjsType*) createTypeVar(ejs, &bootstrap, numSlots, attributes);
    if (type == 0) {
        return 0;
    }

    /*
     *  This will be hand-crafted later
     */
    type->block.obj.var.type = 0;

    return type;
}


static int setTypeProperty(Ejs *ejs, EjsType *type, int slotNum, EjsVar *value)
{
    if (slotNum < 0 && !type->block.obj.var.dynamic) {
        ejsThrowTypeError(ejs, "Object is not dynamic");
        return EJS_ERR;
    }
    return (ejs->blockHelpers->setProperty)(ejs, (EjsVar*) type, slotNum, value);
}


/*
 *  Create a core built-in type. This is used by core native type code to either create a type or to get a type
 *  that has been made by loading ejs.mod. Handles the EMPTY case when building the compiler itself.
 */
EjsType *ejsCreateCoreType(Ejs *ejs, EjsName *qname, EjsType *baseType, int instanceSize, int slotNum, int numTypeProp,
    int numInstanceProp, int attributes)
{
    EjsType     *type;

    type = ejsCreateType(ejs, qname, 0, baseType, instanceSize, slotNum, numTypeProp, numInstanceProp, attributes, 0);
    if (type == 0) {
        ejs->hasError = 1;
        return 0;
    }
    
    /*
     *  The coreTypes hash allows the loader to match the essential core type objects to those being loaded from a mod file.
     */
    mprAddHash(ejs->coreTypes, qname->name, type);

    return type;
}


EjsBlock *ejsCreateTypeInstanceBlock(Ejs *ejs, EjsType* type, int numInstanceProp)
{
    EjsType     *baseType;
    EjsBlock    *block;
    char        *instanceName;
    int         attributes;

    mprAllocSprintf(type, (char**) &instanceName, -1, "%sInstanceType", type->qname.name);

    attributes = 0;
    if (type->block.obj.var.native) {
       attributes |= EJS_ATTR_NATIVE;
    }

    baseType = type->baseType;
    block = createInstanceBlock(ejs, instanceName, (baseType) ? baseType->instanceBlock: 0, numInstanceProp, attributes);
    if (block == 0) {
        return 0;
    }
    ejsSetDebugName(block, block->name);    
    type->instanceBlock = block;
    return block;
}


/*
 *  Create a new type and initialize. BaseType is the super class for instances of the type being created. The
 *  returned EjsType will be an instance of EjsType. numTypeProp and  numInstanceProp should be set to the number
 *  of non-inherited properties.
 */
EjsType *ejsCreateType(Ejs *ejs, EjsName *qname, EjsModule *up, EjsType *baseType, int instanceSize,
                       int slotNum, int numTypeProp, int numInstanceProp, int attributes, void *typeData)
{
    EjsType     *type;
    int         needInstanceBlock;
    
    mprAssert(ejs);
    mprAssert(slotNum >= 0);
    
    needInstanceBlock = numInstanceProp;
    
    if ((ejs->flags & EJS_FLAG_EMPTY) && !ejs->initialized && attributes & EJS_ATTR_NATIVE) {
        /*
         *  If an empty interpreter, must not set a high number of properties based on the last slot generation.
         *  Property counts may be lower or zero this time round.
         */
        numTypeProp = 0;
        numInstanceProp = 0;
    }
    
    type = createType(ejs, qname, up, baseType, instanceSize, numTypeProp, attributes, typeData);
    if (type == 0) {
        return 0;
    }

    type->id = slotNum;
    ejsSetDebugName(type, type->qname.name);

    if (needInstanceBlock) {
        type->instanceBlock = ejsCreateTypeInstanceBlock(ejs, type, numInstanceProp);
    }
    
    if (ejs->globalBlock) {
        type->block.scopeChain = ejs->globalBlock->scopeChain;
    }
    
    return type;
}


/*
 *  Create a type object and initialize.
 */
static EjsType *createType(Ejs *ejs, EjsName *qname, EjsModule *up, EjsType *baseType, int instanceSize, int numSlots, 
        int attributes, void *typeData)
{
    EjsType     *type;

    mprAssert(ejs);
    mprAssert(instanceSize > 0);
    
    /*
     *  Create the type. For Object and Type, the value of ejs->typeType will be null. So bootstrap these first two types. 
     */
    if (ejs->typeType == 0) {
        type = (EjsType*) createBootstrapType(ejs, numSlots, attributes);

    } else {
        type = (EjsType*) createTypeVar(ejs, ejs->typeType, numSlots, attributes);
    }
    if (type == 0) {
        return 0;
    }

    if (baseType) {
        mprAssert(!(attributes & EJS_ATTR_SLOTS_NEED_FIXUP));

        if (baseType->hasConstructor || baseType->hasBaseConstructors) {
            type->hasBaseConstructors = 1;
        }
        if (baseType->hasInitializer || baseType->hasBaseInitializers) {
            type->hasBaseInitializers = 1;
        }
        /* TODO - should we be inheriting staticInitializers? */
        type->baseType = baseType;
    }

    type->qname.name = qname->name;
    type->qname.space = qname->space;
    type->module = up;
    type->typeData = typeData;
    type->baseType = baseType;

    type->instanceSize = (baseType && baseType->instanceSize > instanceSize) ? baseType->instanceSize : instanceSize;

    /*
     *  TODO OPT - should be able to just read in the attributes without having to stuff some in var and some in type.
     *  Should eliminate all the specific fields and just use BIT MASKS.
     */
    type->block.obj.var.native = (attributes & EJS_ATTR_NATIVE) ? 1 : 0;

    if (attributes & EJS_ATTR_SLOTS_NEED_FIXUP) {
        type->needFixup = 1;
    }

    if (attributes & EJS_ATTR_INTERFACE) {
        type->isInterface = 1;
    }
    if (attributes & EJS_ATTR_FINAL) {
        type->final = 1;
    }
    if (attributes & EJS_ATTR_OBJECT) {
        type->hasObject = 1;
    }

    if (attributes & EJS_ATTR_DYNAMIC_INSTANCE) {
        type->dynamicInstance = 1;
    }

    /*
     *  Allocate instance slots inline if dynamic or If a subtype (not Object) is dynamic
     */
    if (attributes & EJS_ATTR_DYNAMIC_INSTANCE || (baseType && baseType != ejs->objectType && baseType->dynamicInstance)) {
        type->separateInstanceSlots = 1;
    }

    if (attributes & EJS_ATTR_HAS_CONSTRUCTOR) {
        /*
         *  This means the type certainly has a constructor method.
         */
        type->hasConstructor = 1;
    }

    if (attributes & EJS_ATTR_HAS_INITIALIZER) {
        type->hasInitializer = 1;
    }
    if (attributes & EJS_ATTR_HAS_STATIC_INITIALIZER) {
        type->hasStaticInitializer = 1;
    }

    if (attributes & EJS_ATTR_CALLS_SUPER) {
        type->callsSuper = 1;
    }
    if (attributes & EJS_ATTR_OPER_OVERLOAD) {
        type->operatorOverload = 1;
    }
    if (attributes & EJS_ATTR_NO_BIND) {
        type->nobind = 1;
    }

    if (attributes & EJS_ATTR_BLOCK_HELPERS) {
        type->helpers = ejsGetBlockHelpers(ejs);

    } else if (attributes & EJS_ATTR_OBJECT_HELPERS) {
        type->helpers = ejsGetObjectHelpers(ejs);

    } else {
        type->helpers = ejsGetDefaultHelpers(ejs);
    }
    
    if (ejsGrowBlock(ejs, &type->block, numSlots) < 0) {
        return 0;
    }
    
    if (baseType && ejsInheritTraits(ejs, (EjsBlock*) type, (EjsBlock*) baseType, baseType->block.numTraits, 0, 0) < 0) {
        return 0;
    }

    return type;
}


/*
 *  Create a type instance block and initialize.
 */
static EjsBlock *createInstanceBlock(Ejs *ejs, cchar *name, EjsBlock *baseBlock, int numSlots, int attributes)
{
    EjsBlock    *block;
    int         oldGen;

    mprAssert(ejs);
    
    /*
     *  Types and instance blocks are always eternal
     */
    oldGen = ejsSetGeneration(ejs, EJS_GEN_ETERNAL);
    block = ejsCreateBlock(ejs, name, numSlots);
    ejsSetGeneration(ejs, oldGen);
    
    if (block == 0) {
        return 0;
    }

    /*
     *  TODO OPT - should be able to just read in the attributes without having to stuff some in var and some in type.
     *  Should eliminate all the specific fields and just use BIT MASKS.
     */
    block->obj.var.native = (attributes & EJS_ATTR_NATIVE) ? 1 : 0;
    block->obj.var.isInstanceBlock = 1;
    
    if (numSlots > 0) {
        if (ejsGrowBlock(ejs, block, numSlots) < 0) {
            return 0;
        }
    
        if (baseBlock && ejsInheritTraits(ejs, (EjsBlock*) block, baseBlock, baseBlock->numTraits, 0, 0) < 0) {
            return 0;
        }
    }

    return block;
}


EjsType *ejsGetType(Ejs *ejs, int slotNum)
{
    EjsType     *type;

    type = (EjsType*) ejsGetProperty(ejs, ejs->global, slotNum);
    if (type == 0 || !ejsIsType(type)) {
        return 0;
    }
    return type;
}


/*
 *  Fixup a type. This is used by the compiler and loader when it must first define a type without knowing the properties of 
 *  base classes. Consequently, it must fixup the type and its counts of inherited properties. It must also copy 
 *  inherited slots and traits. It is also used by the loader to fixup forward class references.
 */
int ejsFixupClass(Ejs *ejs, EjsType *type, EjsType *baseType, MprList *implements, int makeRoom)
{
    mprAssert(ejs);
    mprAssert(type);
    mprAssert(type != baseType);

    type->needFixup = 0;
    type->fixupDone = 1;
    type->baseType = baseType;
    
    if (baseType) {
        if (baseType->hasConstructor || baseType->hasBaseConstructors) {
            type->hasBaseConstructors = 1;
        }
        if (baseType->hasInitializer || baseType->hasBaseInitializers) {
            type->hasBaseInitializers = 1;
        }
        if (baseType != ejs->objectType && baseType->dynamicInstance) {
            type->dynamicInstance = 1;
        }
        type->subTypeCount = baseType->subTypeCount + 1;
    }
    return ejsFixupBlock(ejs, (EjsBlock*) type, (EjsBlock*) baseType, implements, makeRoom);
}


/*
 *  Fixup a block. This is used by the compiler and loader when it must first define a type without knowing the properties 
 *  of base classes. Consequently, it must fixup the type and its counts of inherited properties. It must also copy 
 *  inherited slots and traits. It is also used by the loader to fixup forward class references.
 */
int ejsFixupBlock(Ejs *ejs, EjsBlock *block, EjsBlock *baseBlock, MprList *implements, int makeRoom)
{
    EjsType     *ifaceType;
    EjsBlock    *iface;
    bool        isInstanceBlock;
    int         next, offset, count;

    mprAssert(ejs);
    mprAssert(block);
    mprAssert(block != baseBlock);

    isInstanceBlock = block->obj.var.isInstanceBlock;
    
    if (makeRoom) {
        /*
         *  Count the number of inherited traits and insert
         */
        count = (baseBlock) ? baseBlock->numTraits: 0;
        if (implements) {
            for (next = 0; ((iface = mprGetNextItem(implements, &next)) != 0); ) {
                iface = (isInstanceBlock) ? ((EjsType*) iface)->instanceBlock: iface;
                if (iface) {
                    ifaceType = (EjsType*) iface;
                    if (!ifaceType->isInterface) {
                        /*
                         *  Only inherit properties from implemented classes
                         */
                        count += iface->numTraits - iface->numInherited;
                    }
                }
            }
        }
        if (ejsInsertGrowBlock(ejs, block, count, 0) < 0) {
            return EJS_ERR;
        }
    }

    /*
     *  Copy the inherited traits from the base block and all implemented interfaces
     */
    offset = 0;
    if (baseBlock) {
        if (ejsInheritTraits(ejs, block, baseBlock, baseBlock->numTraits, offset, 0) < 0) {
            return EJS_ERR;
        }
        offset += baseBlock->numTraits;
    }
    
    if (implements) {
        for (next = 0; ((iface = mprGetNextItem(implements, &next)) != 0); ) {
            /*
             *  Only insert the first level of inherited traits
             */
            iface = (isInstanceBlock) ? ((EjsType*) iface)->instanceBlock: iface;
            if (iface) {
                ifaceType = (EjsType*) iface;
                if (!ifaceType->isInterface) {
                    count = iface->numTraits - iface->numInherited;
                    ejsInheritTraits(ejs, block, iface, count, offset, 1);
                    offset += iface->numTraits;
                }
            }
        }
    }

    return 0;
}


/*
 *  Set the native method function for a function property
 */
int ejsBindMethod(Ejs *ejs, EjsType *type, int slotNum, EjsNativeFunction nativeProc)
{
    return ejsBindFunction(ejs, &type->block, slotNum, nativeProc);
}


/*
 *  Set the native method function for a function property
 */
int ejsBindFunction(Ejs *ejs, EjsBlock *block, int slotNum, EjsNativeFunction nativeProc)
{
    EjsFunction     *fun;
    EjsName         qname;

    fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) block, slotNum);

    if (fun == 0 || !ejsIsFunction(fun)) {
        mprAssert(fun);
        ejs->hasError = 1;
        return EJS_ERR;
    }

    if (fun->body.code.codeLen != 0) {
        qname = ejsGetPropertyName(ejs, fun->owner, fun->slotNum);
        mprError(ejs, "Setting a native method on a non-native function %s in block/type %s", qname.name, 
           ejsIsType(block) ? ((EjsType*) block)->qname.name: block->name);
        ejs->hasError = 1;
    }
    fun->body.proc = nativeProc;
    fun->block.obj.var.nativeProc = 1;

    return 0;
}


#if UNUSED
//  TODO - remove and use ejsBindMethod twicew

int ejsSetAccessors(Ejs *ejs, EjsType *type, int getSlot, EjsNativeFunction get, int setSlot, EjsNativeFunction set)
{
    EjsFunction     *fun;

    /*
     *  Define the getter
     */
    if (get) {
        mprAssert(getSlot >= 0);
        fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, getSlot);
        if (fun == 0 || !ejsIsFunction(fun)) {
            mprAssert(0);
            return EJS_ERR;
        }
        fun->proc = get;
        fun->block.obj.var.nativeProc = 1;
    }

    /*
     *  Define the setter
     */
    if (set) {
        mprAssert(setSlot >= 0);
        fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, setSlot);
        if (fun == 0 || !ejsIsFunction(fun)) {
            mprAssert(0);
            return EJS_ERR;
        }
        fun->proc = set;
        fun->block.obj.var.nativeProc = 1;
    }

    return 0;
}
#endif


/*
 *  Define a global public function
 */
int ejsDefineGlobalFunction(Ejs *ejs, cchar *name, EjsNativeFunction fn)
{
    EjsFunction *fun;
    EjsName     qname;

    if ((fun = ejsCreateFunction(ejs, NULL, -1, 0, 0, ejs->objectType, 0, NULL, NULL, 0)) == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    ejsName(&qname, EJS_PUBLIC_NAMESPACE, name);
    return ejsSetPropertyByName(ejs, ejs->global, &qname, (EjsVar*) fun);
}


//  TODO - should there really be a value define?
/*
 *  Define a property. If then property type is null, then use the value's type if supplied. If no value, then set to the 
 *  void type. If OVERRIDE is not set in attributes, then the slotNum is offset above the base class slots.
 */
int ejsDefineInstanceProperty(Ejs *ejs, EjsType *type, int slotNum, EjsName *name, EjsType *propType, int attributes, 
        EjsVar *value)
{
    return ejsDefineProperty(ejs, (EjsVar*) type->instanceBlock, slotNum, name, propType, attributes, value);
}


/*
 *  Return true if target is an instance of type or a sub class of it.
 */
bool ejsIsA(Ejs *ejs, EjsVar *target, EjsType *type)
{
    EjsType     *tp;

    mprAssert(target);
    mprAssert(type);

    if (!ejsIsType(type)) {
        return 0;
    }
    tp = ejsIsType(target) ? (EjsType*) target : target->type;
    return ejsIsTypeSubType(ejs, tp, type);
}


/*
 *  Return true if "target" is a "type", subclass of "type" or implements "type".
 */
bool ejsIsTypeSubType(Ejs *ejs, EjsType *target, EjsType *type)
{
    EjsType     *tp, *iface;
    int         next;

    mprAssert(target);
    mprAssert(type);
    
    if (!ejsIsType(target) || !ejsIsType(type)) {
        return 0;
    }

    /*
     *  See if target is a subtype of type
     */
    for (tp = target; tp; tp = tp->baseType) {
        /*
         *  Test ID also to allow cloned interpreters to match where the IDs are equal
         */
        if (tp == type || tp->id == type->id) {
            return 1;
        }
    }
    
    /*
     *  See if target implements type
     */
    if (target->implements) {
        for (next = 0; (iface = mprGetNextItem(target->implements, &next)) != 0; ) {
            if (iface == type) {
                return 1;
            }
        }
    }

    return 0;
}


/*
 *  Get the attributes of the type property at slotNum.
 *
 */
int ejsGetTypePropertyAttributes(Ejs *ejs, EjsVar *vp, int slotNum)
{
    EjsType     *type;

    if (!ejsIsType(vp)) {
        mprAssert(ejsIsType(vp));
        return EJS_ERR;
    }
    type = (EjsType*) vp;
    return ejsGetTraitAttributes((EjsBlock*) type, slotNum);
}


/*
 *  TODO - This call is currently only used to update the type namespace after resolving a run-time namespace.
 */
void ejsSetTypeName(Ejs *ejs, EjsType *type, EjsName *qname)
{
    type->qname.name = qname->name;
    type->qname.space = qname->space;
    ejsSetDebugName(type, qname->name);
    if (type->instanceBlock) {
        ejsSetDebugName(type->instanceBlock, qname->name);
    }
}


/*
 *  Define namespaces for a class. Inherit the protected and internal namespaces from all base classes.
 */
void ejsDefineTypeNamespaces(Ejs *ejs, EjsType *type)
{
    EjsNamespace        *nsp;

    if (type->baseType) {
        /*
         *  Inherit the base class's protected and internal namespaces
         */
        ejsInheritBaseClassNamespaces(ejs, type, type->baseType);
    }

    nsp = ejsDefineReservedNamespace(ejs, (EjsBlock*) type, &type->qname, EJS_PROTECTED_NAMESPACE);
    nsp->flags |= EJS_NSP_PROTECTED;

    nsp = ejsDefineReservedNamespace(ejs, (EjsBlock*) type, &type->qname, EJS_PRIVATE_NAMESPACE);
    nsp->flags |= EJS_NSP_PRIVATE;
}


void ejsTypeNeedsFixup(Ejs *ejs, EjsType *type)
{
    mprAssert(type);

    type->needFixup = 1;
    type->baseType = 0;
}


/*
 *  Return the total memory size used by a type
 */
static int ejsGetBlockSize(Ejs *ejs, EjsBlock *block)
{
    int     size, numProp;

    numProp = ejsGetPropertyCount(ejs, (EjsVar*) block);

    size = sizeof(EjsType) + sizeof(EjsTypeHelpers) + (numProp * sizeof(EjsVar*));
    if (block->obj.names) {
        size += sizeof(EjsNames) + (block->obj.names->sizeEntries * sizeof(EjsHashEntry));
        size += (block->obj.names->sizeBuckets * sizeof(int*));
    }
    size += ejsGetNumTraits(block) * sizeof(EjsTrait);

    return size;
}


/*
 *  Return the total memory size used by a type
 */
int ejsGetTypeSize(Ejs *ejs, EjsType *type)
{
    int     size;

    size = ejsGetBlockSize(ejs, (EjsBlock*) type);
    if (type->instanceBlock) {
        size += ejsGetBlockSize(ejs, type->instanceBlock);
    }
    return size;
}



void ejsCreateTypeType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;
    int         flags;

    flags = EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_BLOCK_HELPERS;
    if (ejs->flags & EJS_FLAG_EMPTY) {
        flags |= EJS_ATTR_DYNAMIC_INSTANCE;
    }

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Type"), ejs->objectType, sizeof(EjsType), 
        ES_Type, ES_Type_NUM_CLASS_PROP, ES_Type_NUM_INSTANCE_PROP, flags);
    ejs->typeType = type;

    /*
     *  Override the createVar helper when creating types.
     */
    type->helpers->cloneVar     = (EjsCloneVarHelper) cloneTypeVar;
    type->helpers->createVar    = (EjsCreateVarHelper) createTypeVar;
    type->helpers->setProperty  = (EjsSetPropertyHelper) setTypeProperty;

    /*
     *  WARNING: read closely. This can be confusing. Fixup the helpers for the object type. We need to find
     *  helpers via objectType->var.type->helpers. So we set it to the Type type. We keep objectType->baseType == 0
     *  because Object has no base type. Similarly for the Type type.
     */
    ejs->objectType->block.obj.var.type = ejs->typeType;
    ejs->typeType->block.obj.var.type = ejs->objectType;
}


void ejsConfigureTypeType(Ejs *ejs)
{
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsType.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/core/ejsVoid.c"
 */
/************************************************************************/

/**
 *  ejsVoid.c - Ejscript Void class (aka undefined)
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Cast the void operand to a primitive type
 */

static EjsVar *castVoid(Ejs *ejs, EjsVoid *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->falseValue;

    case ES_Number:
        return (EjsVar*) ejs->nanValue;

    case ES_Object:
        return vp;

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "undefined");

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}



static EjsVar *coerceVoidOperands(Ejs *ejs, EjsVoid *lhs, int opcode, EjsVoid *rhs)
{
    switch (opcode) {

    case EJS_OP_ADD:
        if (!ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        /* Fall through */

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->nanValue, opcode, rhs);

    /*
     *  Comparision
     */
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        if (ejsIsNull(rhs)) {
            return (EjsVar*) ejs->falseValue;
        }
        return (EjsVar*) ejs->trueValue;


    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
        if (ejsIsNull(rhs)) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}



static EjsVar *invokeVoidOperator(Ejs *ejs, EjsVoid *lhs, int opcode, EjsVoid *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceVoidOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types match, left and right types are both "undefined"
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->nanValue;

    /*
     *  Binary operators
     */
    case EJS_OP_ADD: case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return (EjsVar*) ejs->nanValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


/*
 *  iterator native function get(): Iterator
 */
static EjsVar *getVoidIterator(Ejs *ejs, EjsVar *np, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, np, NULL, 0, NULL);
}


/*
 *  We don't actually create any instances. We just use a reference to the undefined singleton instance.
 */
EjsVoid *ejsCreateUndefined(Ejs *ejs)
{
    return (EjsVoid*) ejs->undefinedValue;
}



void ejsCreateVoidType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Void"), ejs->objectType, sizeof(EjsVoid),
        ES_Void, ES_Void_NUM_CLASS_PROP, ES_Void_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->voidType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castVoid;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeVoidOperator;

    ejs->undefinedValue = ejsCreateVar(ejs, type, 0);
    ejsSetDebugName(ejs->undefinedValue, "undefined");

    if (!(ejs->flags & EJS_FLAG_EMPTY)) {
        /*
         *  Define the "undefined" value
         */
        ejsSetProperty(ejs, ejs->global, ES_undefined, ejs->undefinedValue);
    }
}


void ejsConfigureVoidType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->voidType;

    ejsSetProperty(ejs, ejs->global, ES_undefined, ejs->undefinedValue);

    ejsBindMethod(ejs, type, ES_Object_get, getVoidIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getVoidIterator);
}



/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/core/ejsVoid.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/events/ejsTimer.c"
 */
/************************************************************************/

/*
 *  ejsTimer.c -- Timer class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static void timerCallback(EjsTimer *tp, MprEvent *e);

/*
 *  Create a new timer
 *
 *  function Timer(period: Number, callback: Function, drift: Boolean = true)
 */
static EjsVar *constructor(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 2 || argc == 3);
    mprAssert(ejsIsNumber(argv[0]));
    mprAssert(ejsIsFunction(argv[1]));

    tp->ejs = ejs;
    tp->period = ejsGetInt(argv[0]);
    tp->callback = (EjsFunction*) argv[1];
    tp->drift = (argc == 3) ? ejsGetInt(argv[2]) : 1;

    tp->event = mprCreateTimerEvent(ejs, (MprEventProc) timerCallback, tp->period, MPR_NORMAL_PRIORITY, tp, MPR_EVENT_CONTINUOUS);
    if (tp->event == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    return 0;
}



/*
 *  Get the timer drift setting
 *
 *  function get drift(): Boolean
 */
static EjsVar *getDrift(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    return (EjsVar*) ejsCreateBoolean(ejs, tp->drift);
}


/*
 *  Set the timer drift setting
 *
 *  function set drift(period: Boolean): Void
 */
static EjsVar *setDrift(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsBoolean(argv[0]));

    tp->drift = ejsGetBoolean(argv[0]);
    return 0;
}


/*
 *  Get the timer period
 *
 *  function get period(): Number
 */
static EjsVar *getPeriod(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    return (EjsVar*) ejsCreateNumber(ejs, tp->period);
}


/*
 *  Set the timer period and restart the timer
 *
 *  function set period(period: Number): Void
 */
static EjsVar *setPeriod(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    tp->period = ejsGetInt(argv[0]);
    mprRescheduleEvent(tp->event, tp->period);
    return 0;
}


/*
 *  Restart a timer
 *
 *  function restart(); Void
 */
static EjsVar *restart(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    mprRestartContinuousEvent(tp->event);
    return 0;
}


/*
 *  Stop a timer
 *
 *  function stop(): Void
 */
static EjsVar *stop(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    mprRemoveEvent(tp->event);
#if UNUSED
    mprStopContinuousEvent(tp->event);
#endif
    return 0;
}



EjsObject *ejsCreateTimerEvent(Ejs *ejs, EjsTimer *tp)
{
    EjsObject       *event;

    event = ejsCreateObject(ejs, ejsGetType(ejs, ES_ejs_events_TimerEvent), 0);
    if (event == 0) {
        return 0;
    }
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_data, (EjsVar*) tp);
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_timestamp, (EjsVar*) ejsCreateDate(ejs, 0));
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_bubbles, (EjsVar*) ejs->falseValue);
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_priority, (EjsVar*) ejsCreateNumber(ejs, MPR_NORMAL_PRIORITY));
    return event;
}



static void timerCallback(EjsTimer *tp, MprEvent *e)
{
    Ejs         *ejs;
    EjsObject   *event;
    EjsVar      *arg;

    mprAssert(tp);

    ejs = tp->ejs;

    event = ejsCreateTimerEvent(ejs, tp);
    if (event == 0) {
        return;
    }

    arg = (EjsVar*) event;
    ejsRunFunction(tp->ejs, tp->callback, 0, 1, &arg);
}



void ejsCreateTimerType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.events", "Timer"), ejs->objectType, sizeof(EjsTimer), 
        ES_ejs_events_Timer, ES_ejs_events_Timer_NUM_CLASS_PROP, ES_ejs_events_Timer_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureTimerType(Ejs *ejs)
{
    EjsType     *type;

    type = ejsGetType(ejs, ES_ejs_events_Timer);

    ejsBindMethod(ejs, type, ES_ejs_events_Timer_Timer, (EjsNativeFunction) constructor);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_restart, (EjsNativeFunction) restart);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_stop, (EjsNativeFunction) stop);

    ejsBindMethod(ejs, type, ES_ejs_events_Timer_period, (EjsNativeFunction) getPeriod);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_set_period, (EjsNativeFunction) setPeriod);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_drift, (EjsNativeFunction) getDrift);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_set_drift, (EjsNativeFunction) setDrift);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/events/ejsTimer.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/io/ejsFile.c"
 */
/************************************************************************/

/**
 *  ejsFile.c - File class.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




#if BLD_WIN_LIKE
#define isDelim(fp, c)  (c == '/' || c == fp->delimiter)
#else
#define isDelim(fp, c)  (c == fp->delimiter)
#endif


static int  readData(Ejs *ejs, EjsFile *fp, EjsByteArray *ap, int offset, int count);

#if BLD_FEATURE_MMU && FUTURE
static void *mapFile(EjsFile *fp, uint size, int mode);
static void unmapFile(EjsFile *fp);
#endif

/*
 *  Index into a file and extract a byte. This is random access reading.
 */
static EjsVar *getFileProperty(Ejs *ejs, EjsFile *fp, int slotNum)
{
    int     c, offset;

    if (!(fp->mode & EJS_FILE_OPEN)) {
        ejsThrowIOError(ejs, "File is not open");
        return 0;
    }
#if UNUSED
    if (fp->mode & EJS_FILE_READ) {
        if (slotNum >= fp->info.size) {
            ejsThrowOutOfBoundsError(ejs, "Bad file index");
            return 0;
        }
    }
    if (slotNum < 0) {
        ejsThrowOutOfBoundsError(ejs, "Bad file index");
        return 0;
    }
#endif

#if BLD_FEATURE_MMU && FUTURE
    //  TODO - must check against mapped size here.
    c = fp->mapped[slotNum];
#else
    offset = mprSeek(fp->file, SEEK_CUR, 0);
    if (offset != slotNum) {
        if (mprSeek(fp->file, SEEK_SET, slotNum) != slotNum) {
            ejsThrowIOError(ejs, "Can't seek to file offset");
            return 0;
        }
    }
    c = mprPeekc(fp->file);
    if (c < 0) {
        ejsThrowIOError(ejs, "Can't read file");
        return 0;
    }
#endif
    return (EjsVar*) ejsCreateNumber(ejs, c);
}


/*
 *  Set a byte in the file at the offset designated by slotNum.
 */
static int setFileProperty(Ejs *ejs, EjsFile *fp, int slotNum, EjsVar *value)
{
    int     c, offset;

    if (!(fp->mode & EJS_FILE_OPEN)) {
        ejsThrowIOError(ejs, "File is not open");
        return 0;
    }
    if (!(fp->mode & EJS_FILE_WRITE)) {
        ejsThrowIOError(ejs, "File is not opened for writing");
        return 0;
    }
    c = ejsIsNumber(value) ? ejsGetInt(value) : ejsGetInt(ejsToNumber(ejs, value));

    offset = mprSeek(fp->file, SEEK_CUR, 0);
    if (slotNum < 0) {
        //  TODO OPT - could have an mprGetPosition(file) API
        slotNum = offset;
    }

#if BLD_FEATURE_MMU && FUTURE
    fp->mapped[slotNum] = c;
#else
    if (offset != slotNum && mprSeek(fp->file, SEEK_SET, slotNum) != slotNum) {
        ejsThrowIOError(ejs, "Can't seek to file offset");
        return 0;
    }

    if (mprPutc(fp->file, c) < 0) {
        ejsThrowIOError(ejs, "Can't write file");
        return 0;
    }
#endif
    return slotNum;
}


/*
 *  Constructor
 *
 *  function File(path: String)
 *
 */
static EjsVar *fileConstructor(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *path;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    path = ejsGetString(argv[0]);
    fp->path = mprCleanFilename(fp, path);

    return (EjsVar*) fp;
}


/*
 *  Return an absolute path name for the file
 *
 *  function get absolutePath()
 */
static EjsVar *absolutePath(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsVar  *vp;
    char    *path;

    path = mprGetAbsFilename(fp, fp->path);
    vp = (EjsVar*) ejsCreateString(ejs, path);
    mprFree(path);
    return vp;
}


/*
 *  Get the base name of a file
 *
 *  function basename(): String
 */
static EjsVar *basenameProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetBaseName(fp->path));
}


/*
 *  Close the file and free up all associated resources.
 *
 *  function close(graceful: Boolean): void
 */
static EjsVar *closeFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    if (fp->mode & EJS_FILE_OPEN && fp->mode & EJS_FILE_WRITE) {
        if (mprFlush(fp->file) < 0) {
            ejsThrowIOError(ejs, "Can't flush file data");
            return 0;
        }
    }

    if (fp->file) {
        mprFree(fp->file);
        fp->file = 0;
    }
#if BLD_FEATURE_MMU && FUTURE
    if (fp->mapped) {
        unmapFile(fp);
        fp->mapped = 0;
    }
#endif
    return 0;
}


/*
 *  Copy a file
 *
 *  function copy(to: String): Void
 */
static EjsVar *copyFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *from, *to;
    char        *buf, *toPath;
    uint        bytes;
    int         rc;

    mprAssert(argc == 1);
    toPath = ejsGetString(argv[0]);

    from = mprOpen(ejs, fp->path, O_RDONLY | O_BINARY, 0);
    if (from == 0) {
        ejsThrowIOError(ejs, "Cant open %s", fp->path);
        return 0;
    }

    to = mprOpen(ejs, toPath, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0664);
    if (to == 0) {
        ejsThrowIOError(ejs, "Cant create %s", toPath);
        mprFree(from);
        return 0;
    }

    buf = mprAlloc(ejs, MPR_BUFSIZE);
    if (buf == 0) {
        ejsThrowMemoryError(ejs);
        mprFree(to);
        mprFree(from);
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(from, buf, MPR_BUFSIZE)) > 0) {
        if (mprWrite(to, buf, bytes) != bytes) {
            ejsThrowIOError(ejs, "Write error to %s", toPath);
            rc = 0;
            break;
        }
    }

    mprFree(from);
    mprFree(to);
    mprFree(buf);

    return 0;
}


/*
 *  Create a temporary file. Creates a new, uniquely named temporary file.
 *
 *  static function createTempFile(directory: String = null): File
 */
static EjsVar *createTempFile(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    char    *directory, path[MPR_MAX_FNAME];

    mprAssert(argc == 0 || argc == 1);

    directory = (argc == 1) ? ejsGetString(argv[0]): NULL;

    if (mprMakeTempFileName(ejs, path, sizeof(path), directory) < 0) {
        ejsThrowIOError(ejs, "Can't make temp file");
        return 0;
    }

    return (EjsVar*) ejsCreateFile(ejs, path);
}


/*
 *  Return when the file was created.
 *
 *  function get created(): Date
 */
static EjsVar *created(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) info.ctime);
}


/**
 *  Get the directory name portion of a file.
 *
 *  function dirname(): String
 */
static EjsVar *dirnameProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *dir;
    int     len;

    len = (int) strlen(fp->path) + 1;
    dir = mprAlloc(fp, len);
    if (dir == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    return (EjsVar*) ejsCreateString(ejs, mprGetDirName(dir, len, fp->path));
}


/*
 *  Test to see if this file exists.
 *
 *  function get exists(): Boolean
 */
static EjsVar *exists(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    return (EjsVar*) ejsCreateBoolean(ejs, mprGetFileInfo(ejs, fp->path, &info) == 0);
}


/*
 *  Get the file extension portion of the file name.
 *
 *  function get extension(): String
 */
static EjsVar *extension(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *cp;

    if ((cp = strrchr(fp->path, '.')) == 0) {
        return (EjsVar*) ejs->emptyStringValue;
    }
    return (EjsVar*) ejsCreateString(ejs, cp);
}


/*
 *  Flush the stream and the underlying file data
 *
 *  function flush(): void
 */
static EjsVar *flushFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    if (!(fp->mode & EJS_FILE_OPEN)) {
        ejsThrowIOError(ejs, "File is not open");
        return 0;
    }
    if (!(fp->mode & EJS_FILE_WRITE)) {
        ejsThrowIOError(ejs, "File is not opened for writing");
        return 0;
    }
    if (mprFlush(fp->file) < 0) {
        ejsThrowIOError(ejs, "Can't flush file data");
        return 0;
    }
    return 0;
}


/*
 *  Return the amount of free space in the file system that would contain the given path.
 *  function freeSpace(path: String = null): Number
 */
static EjsVar *freeSpace(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    //  TODO

#if BREW
    Mpr     *mpr;
    uint    space;

    mpr = mprGetMpr(ejs);
    space = IFILEMGR_GetFreeSpace(mpr->fileMgr, 0);
    ejsSetReturnValueToInteger(ejs, space);
#endif

    return 0;
}


/*
 *  Function to iterate and return the next element index.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsFile     *fp;

    fp = (EjsFile*) ip->target;
    if (!ejsIsFile(fp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < fp->info.size) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator for use with "for ... in". This returns byte offsets in the file.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getFileIterator(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprGetFileInfo(ejs, fp->path, &fp->info);
    return (EjsVar*) ejsCreateIterator(ejs, (EjsVar*) fp, (EjsNativeFunction) nextKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsFile     *fp;

    fp = (EjsFile*) ip->target;
    if (!ejsIsFile(fp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < fp->info.size) {
#if !BLD_FEATURE_MMU || 1
        if (mprSeek(fp->file, SEEK_CUR, 0) != ip->index) {
            if (mprSeek(fp->file, SEEK_SET, ip->index) != ip->index) {
                ejsThrowIOError(ejs, "Can't seek to %d", ip->index);
                return 0;
            }
        }
        ip->index++;
        return (EjsVar*) ejsCreateNumber(ejs, mprGetc(fp->file));
#else
        return (EjsVar*) ejsCreateNumber(ejs, fp->mapped[ip->index++]);
#endif
    }

#if BLD_FEATURE_MMU && FUTURE
    unmapFile(fp);
    fp->mapped = 0;
#endif

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to enumerate the bytes in the file. For use with "for each ..."
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getValues(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprGetFileInfo(ejs, fp->path, &fp->info);

    return (EjsVar*) ejsCreateIterator(ejs, (EjsVar*) fp, (EjsNativeFunction) nextValue, 0, NULL);
}


/*
 *  Get the files in a directory.
 *  function getFiles(enumDirs: Boolean = false): Array
 *
 *  TODO -- need pattern to match (what about "." and ".." and ".*")
 *  TODO - move this functionality into mprFile (see appweb dirHandler.c)
 */
static EjsVar *getFiles(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char            path[MPR_MAX_FNAME];
    EjsArray        *array;
    MprList         *list;
    MprDirEntry     *dp;
    bool            enumDirs, noPath;
    int             next;

    mprAssert(argc == 0 || argc == 1);

    enumDirs = (argc == 1) ? ejsGetBoolean(argv[0]): 0;

    array = ejsCreateArray(ejs, 0);
    if (array == 0) {
        //  TODO - great to push this down into ejsAllocVar
        ejsThrowMemoryError(ejs);
        return 0;
    }

    list = mprGetDirList(array, fp->path, enumDirs);
    if (list == 0) {
        ejsThrowIOError(ejs, "Can't read directory");
        return 0;
    }

    noPath = (fp->path[0] == '.' && fp->path[1] == '\0') || (fp->path[0] == '.' && fp->path[1] == '/' && fp->path[2] == '\0');

    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        if (strcmp(dp->name, ".") == 0 || strcmp(dp->name, "..") == 0) {
            continue;
        }
        if (enumDirs || !(dp->isDir)) {
            if (noPath) {
                ejsSetProperty(ejs, (EjsVar*) array, -1, (EjsVar*) ejsCreateString(ejs, dp->name));
            } else {
                /*
                 *  Prepend the directory name
                 */
                mprSprintf(path, sizeof(path), "%s/%s", fp->path,  dp->name);
                ejsSetProperty(ejs, (EjsVar*) array, -1, (EjsVar*) ejsCreateString(ejs, path));
            }
        }
    }
    mprFree(list);

    return (EjsVar*) array;
}


/*
 *  Get the file contents as a byte array
 *
 *  static function getBytes(path: String): ByteArray
 */
static EjsVar *getBytes(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile         *file;
    EjsByteArray    *result;
    cchar           *path;
    char            buffer[MPR_BUFSIZE];
    int             bytes, offset, rc;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    path = ejsGetString(argv[0]);

    file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open %s", path);
        return 0;
    }

    /*
     *  TODO - need to be smarter about running out of memory here if the file is very large.
     */
    result = ejsCreateByteArray(ejs, (int) mprGetFileSize(file));
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    rc = 0;
    offset = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (ejsCopyToByteArray(ejs, result, offset, buffer, bytes) < 0) {
            ejsThrowMemoryError(ejs);
            rc = -1;
            break;
        }
        offset += bytes;
    }
    ejsSetByteArrayPositions(ejs, result, 0, offset);

    mprFree(file);

    return (EjsVar*) result;
}


/**
 *  Get the file contents as an array of lines.
 *
 *  static function getLines(path: String): Array
 */
static EjsVar *getLines(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *file;
    MprBuf      *data;
    EjsArray    *result;
    cchar       *path;
    char        *start, *end, *cp, buffer[MPR_BUFSIZE];
    int         bytes, rc, lineno;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    path = ejsGetString(argv[0]);

    result = ejsCreateArray(ejs, 0);
    if (result == NULL) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open %s", path);
        return 0;
    }

    /*
     *  TODO - need to be smarter about running out of memory here if the file is very large.
     */
    data = mprCreateBuf(ejs, 0, (int) mprGetFileSize(file) + 1);
    result = ejsCreateArray(ejs, 0);
    if (result == NULL || data == NULL) {
        ejsThrowMemoryError(ejs);
        mprFree(file);
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (mprPutBlockToBuf(data, buffer, bytes) != bytes) {
            ejsThrowMemoryError(ejs);
            rc = -1;
            break;
        }
    }

    start = mprGetBufStart(data);
    end = mprGetBufEnd(data);
    for (lineno = 0, cp = start; cp < end; cp++) {
        if (*cp == '\n') {
            if (ejsSetProperty(ejs, (EjsVar*) result, lineno++, 
                    (EjsVar*) ejsCreateStringWithLength(ejs, start, (int) (cp - start))) < 0) {
                break;
            }
            start = cp + 1;
        } else if (*cp == '\r') {
            start = cp + 1;
        }
    }
    if (cp > start) {
        ejsSetProperty(ejs, (EjsVar*) result, lineno++, (EjsVar*) ejsCreateStringWithLength(ejs, start, (int) (cp - start)));
    }

    mprFree(file);
    mprFree(data);

    return (EjsVar*) result;
}


/**
 *  Get the file contents as a string
 *
 *  static function getString(path: String): String
 */
static EjsVar *getFileAsString(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *file;
    MprBuf      *data;
    EjsVar      *result;
    cchar       *path;
    char        buffer[MPR_BUFSIZE];
    int         bytes, rc;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    path = ejsGetString(argv[0]);

    file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open %s", path);
        return 0;
    }

    /*
     *  TODO - need to be smarter about running out of memory here if the file is very large.
     */
    data = mprCreateBuf(ejs, 0, (int) mprGetFileSize(file) + 1);
    if (data == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (mprPutBlockToBuf(data, buffer, bytes) != bytes) {
            ejsThrowMemoryError(ejs);
            rc = -1;
            break;
        }
    }

    result = (EjsVar*) ejsCreateStringWithLength(ejs, mprGetBufStart(data),  mprGetBufLength(data));

    mprFree(file);
    mprFree(data);

    return result;
}


/*
 *  Get the file contents as an XML object
 *
 *  static function getXML(path: String): XML
 */
static EjsVar *getXML(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return 0;
}


/*
 *  Determine if the file path has a drive spec (C:) in the file name
 *
 *  static function hasDriveSpec(): Boolean
 */
static EjsVar *hasDriveSpec(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, 
        (isalpha((int) fp->path[0]) && fp->path[1] == ':' && (fp->path[2] == '/' || fp->path[2] == '\\')));
}


/*
 *  Determine if the file name is a directory
 *
 *  function get isDir(): Boolean
 */
static EjsVar *isDir(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;
    int             rc;

    rc = mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateBoolean(ejs, rc == 0 && info.isDir);
}


/*
 *  Determine if the file is currently open for reading or writing
 *
 *  function get isOpen(): Boolean
 */
static EjsVar *isOpen(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, fp->file != NULL);
}


/*
 *  Determine if the file name is a regular file
 *
 *  function get isRegular(): Boolean
 */
static EjsVar *isRegular(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateBoolean(ejs, info.isReg);
}


/*
 *  Get when the file was last accessed.
 *
 *  function get lastAccess(): Date
 */
static EjsVar *lastAccess(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) info.atime);
}


/*
 *  Get the length of the file associated with this File object.
 *
 *  override intrinsic function get length(): Number
 */
static EjsVar *fileLength(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    if (fp->mode & EJS_FILE_OPEN) {
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) mprGetFileSize(fp->file));

    } else {
        if (mprGetFileInfo(ejs, fp->path, &info) < 0) {
            return (EjsVar*) ejs->minusOneValue;
        }
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) info.size);
    }
}


/*
 *  function makeDir(permissions: Number = 0755): Void
 */
static EjsVar *makeDir(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;
    int             perms;

    mprAssert(argc == 0 || argc == 1);

    perms = (argc == 1) ? ejsGetInt(argv[0]) : 0755;

    if (mprGetFileInfo(ejs, fp->path, &info) == 0 && info.isDir) {
        return 0;
    }
    if (mprMakeDirPath(ejs, fp->path, perms) < 0) {
        ejsThrowIOError(ejs, "Cant create directory %s", fp->path);
        return 0;
    }
    return 0;
}


/**
 *  Get the file access mode.
 *
 *  function get mode(): Number
 */
static EjsVar *mode(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, (fp->file) ? fp->mode: 0);
}


/*
 *  Get when the file was created or last modified.
 *
 *  function get modified(): Date
 */
static EjsVar *modified(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) info.mtime);
}


/*
 *  Get the name of the file associated with this File object.
 *
 *  function get name(): String
 */
static EjsVar *name(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, fp->path);
}


/*
 *  Get the newline characters
 *
 *  function get newline(): String
 */
static EjsVar *newline(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetFileNewline(ejs, "/"));
}


/*
 *  set the newline characters
 *
 *  function set newline(terminator: String): Void
 */
static EjsVar *setNewline(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprAssert(ejsIsString(argv[0]));
    mprSetFileNewline(ejs, "/", ((EjsString*) argv[0])->value);
    return 0;
}


/*
 *  Open a file using the current file name. This method requires a file instance.
 *
 *  function open(mode: Number = Read, permissions: Number = 0644): void
 */
static EjsVar *openProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    int     flags, mode, perms;

    mode = (argc >= 1) ? ejsGetInt(argv[0]) : EJS_FILE_READ;
    perms = (argc == 2) ? ejsGetInt(argv[1]) : 0644;

    if (fp->file) {
        ejsThrowIOError(ejs, "File already open");
        return 0;
    }

    flags = 0;

    if (mode & EJS_FILE_WRITE) {
        //  TODO - read/write needs more thought
        flags = O_BINARY | ((mode & EJS_FILE_READ) ? O_RDWR : O_WRONLY);
        if (mode & EJS_FILE_TRUNCATE) {
            flags |= O_TRUNC;
        }
        if (mode & EJS_FILE_APPEND) {
            flags |= O_APPEND;
        }
        if (mode & EJS_FILE_CREATE) {
            flags |= O_CREAT;
        }
        fp->file = mprOpen(ejs, fp->path, flags, perms);

    } else {
        /*
         *  Read is the default
         */
        flags = O_RDONLY;
        fp->file = mprOpen(ejs, fp->path, O_RDONLY | O_BINARY, 0);
    }

    if (fp->file == 0) {
        fp->mode = 0;
        ejsThrowIOError(ejs, "Can't open %s, error %d", fp->path, errno);
    } else {
        fp->mode = mode | EJS_FILE_OPEN;
    }
    //  TODO - should push this into File and it should maintain position and length
    mprGetFileInfo(ejs, fp->path, &fp->info);

#if BLD_FEATURE_MMU && FUTURE
    //  TODO - should always do info at the start.
    //  TODO - could push this into mprOpen via:  O_MAPPED. Need destructor to unmap.
    fp->mapped = mapFile(fp, fp->info.size, MPR_MAP_READ | MPR_MAP_WRITE);
#endif

    return 0;
}


/*
 *  Get the parent directory of the absolute path of the file.
 *
 *  function get parent(): String
 */
static EjsVar *parent(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsVar  *vp;
    char    *path;

    path = mprGetParentDir(fp, fp->path);
    vp = (EjsVar*) ejsCreateString(ejs, path);
    mprFree(path);
    return vp;
}


//  TODO - needs a path arg.
/*
 *  Return the path segment delimiter
 *
 *  static function get pathDelimiter(): String
 */
static EjsVar *pathDelimiter(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    buf[2];

    buf[0] = mprGetFileDelimiter(ejs, "/");
    buf[1] = '\0';
    return (EjsVar*) ejsCreateString(ejs, buf);
}


//  TODO - Remove.  This is defined by the file system and not really modifiable.
/*
 *  Set the path segment delimiter
 *
 *  static function set pathDelimiter(value: String): void
 */
static EjsVar *setPathDelimiter(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsString(argv[0]));
    mprSetFileDelimiter(ejs, "/", ((EjsString*) argv[0])->value[0]);
    return 0;
}


/*
 *  Get the file security permissions.
 *
 *  function get permissions(): Number
 */
static EjsVar *getPermissions(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateNumber(ejs, info.perms);
}


/*
 *  Set the file security permissions.
 *
 *  function set permissions(mask: Number): void
 */
static EjsVar *setPermissions(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
#if !VXWORKS
    int     perms;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    perms = ejsGetInt(argv[0]);
//  TODO - windows requires _chmod
    if (chmod(fp->path, perms) < 0) {
        ejsThrowIOError(ejs, "Can't set permissions on %s", fp->path);
    }
#endif
    return 0;
}


/*
 *  Get the current I/O position in the file.
 *
 *  function get position(): Number
 */
static EjsVar *getPosition(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not opened");
        return 0;
    }
    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) mprGetFilePosition(fp->file));
}


/*
 *  Seek to a new location in the file and set the File marker to a new read/write position.
 *
 *  function set position(value: Number): void
 */
static EjsVar *setPosition(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    long        pos;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    pos = ejsGetInt(argv[0]);

    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not opened");
        return 0;
    }
    pos = ejsGetInt(argv[0]);
    if (mprSeek(fp->file, SEEK_SET, pos) != pos) {
        ejsThrowIOError(ejs, "Can't seek to %ld", pos);
    }
    return 0;
}


/*
 *  Put the file contents
 *
 *  static function put(path: String, permissions: Number, ...args): void
 */
static EjsVar *putToFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *file;
    EjsArray    *args;
    char        *path, *data;
    int         i, bytes, length, permissions;

    mprAssert(argc == 3);

    path = ejsGetString(argv[0]);
    permissions = ejsGetInt(argv[1]);
    args = (EjsArray*) argv[2];

    /*
     *  Create fails if already present
     */
    mprDelete(ejs, path);
    file = mprOpen(ejs, path, O_CREAT | O_WRONLY | O_BINARY, permissions);
    if (file == 0) {
        ejsThrowIOError(ejs, "Cant create %s", path);
        return 0;
    }

    for (i = 0; i < args->length; i++) {
        data = ejsGetString(ejsToString(ejs, ejsGetProperty(ejs, (EjsVar*) args, i)));
        length = (int) strlen(data);
        bytes = mprWrite(file, data, length);
        if (bytes != length) {
            ejsThrowIOError(ejs, "Write error to %s", path);
            break;
        }
    }
    mprFree(file);

    return 0;
}


/*
 *  Return a relative path name for the file.
 *
 *  function get relativePath(): String
 */
static EjsVar *relativePath(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetRelFilename(fp, fp->path));
}


/*
 *  Read data bytes from a file
 *
 *  function readBytes(count: Number): ByteArray
 */
static EjsVar *readBytes(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsByteArray    *result;
    MprFileInfo     info;
    int             count, arraySize, totalRead;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    count = ejsGetInt(argv[0]);

    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not open");
        return 0;
    }

    if (!(fp->mode & EJS_FILE_READ)) {
        ejsThrowStateError(ejs, "File not opened for reading");
        return 0;
    }

    arraySize = mprGetFileInfo(fp, fp->path, &info) == 0 ? (int) info.size : MPR_BUFSIZE;
    result = ejsCreateByteArray(ejs, arraySize);
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    /*
     *  TODO - what if the file is opened with a stream. Should still work
     */
    totalRead = readData(ejs, fp, result, 0, count);
    if (totalRead < 0) {
        return 0;
    }
    ejsSetByteArrayPositions(ejs, result, 0, totalRead);

    return (EjsVar*) result;
}


/*
 *  Read data bytes from a file
 *
 *  function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
 */
static EjsVar *readProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsByteArray    *buffer;
    int             count, offset, totalRead;

    mprAssert(1 <= argc && argc <= 3);

    buffer = (EjsByteArray*) argv[0];
    offset = (argc >= 2) ? ejsGetInt(argv[1]): 0;
    count = (argc >= 3) ? ejsGetInt(argv[2]): buffer->length;

    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not open");
        return 0;
    }

    if (!(fp->mode & EJS_FILE_READ)) {
        ejsThrowStateError(ejs, "File not opened for reading");
        return 0;
    }

    /*
     *  TODO - what if the file is opened with a stream. Should still work
     */
    totalRead = readData(ejs, fp, buffer, offset, count);
    if (totalRead < 0) {
        return 0;
    }
    ejsSetByteArrayPositions(ejs, buffer, offset, totalRead);

    return (EjsVar*) ejsCreateNumber(ejs, totalRead);
}


/*
 *  function removeDir(): Void
 */
static EjsVar *removeDir(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    if (mprGetFileInfo(ejs, fp->path, &info) == 0 && mprDeleteDir(ejs, fp->path) < 0) {
        ejsThrowIOError(ejs, "Cant remove directory %s", fp->path);
        return 0;
    }
    return 0;
}


/*
 *  Remove the file associated with the File object.
 *
 *  function remove(quiet: Boolean = true): void
 */
static EjsVar *removeFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    if (fp->file) {
        ejsThrowStateError(ejs, "Can't remove an open file");
        return 0;
    }
    if (mprGetFileInfo(ejs, fp->path, &info) == 0 && unlink(fp->path) < 0) {
        ejsThrowIOError(ejs, "Cant remove file %s", fp->path);
    }
    return 0;
}


/*
 *  Rename the file
 *
 *  function rename(to: String): Void
 */
static int renameProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *to;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    to = ejsGetString(argv[0]);

    unlink(to);
    if (rename(fp->path, to) < 0) {
        ejsThrowIOError(ejs, "Cant rename file %s to %s", fp->path, to);
        return 0;
    }
    return 0;
}


/*
 *  Put the file stream into async mode and define a completion callback
 *
 *  function setCallback(callback: Function): void
 */
static EjsVar *setFileCallback(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprAssert(0);
    return 0;
}


/*
 *  Return an absolute unix style path name for the file (not drive spec)
 *
 *  function get unixPath(): String
 */
static EjsVar *unixPath(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsVar  *vp;
    char    *path;

    path = mprGetUnixFilename(fp, fp->path);
    vp = (EjsVar*) ejsCreateString(ejs, path);
    mprFree(path);

    return (EjsVar*) vp;
}


/*
 *  Write data to the file
 *
 *  function write(data: Object): Number
 */
EjsVar *fileWrite(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsArray        *args;
    EjsByteArray    *ap;
    EjsVar          *vp;
    EjsString       *str;
    char            *buf;
    int             i, len, written;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    if (!(fp->mode & EJS_FILE_WRITE)) {
        ejsThrowStateError(ejs, "File not opened for writing");
        return 0;
    }

    written = 0;

    for (i = 0; i < args->length; i++) {
        vp = ejsGetProperty(ejs, (EjsVar*) args, i);
        mprAssert(vp);
        switch (vp->type->id) {
        case ES_ByteArray:
            //  TODO - should have function for this
            ap = (EjsByteArray*) vp;
            buf = (char*) &ap->value[ap->readPosition];
            len = ap->writePosition - ap->readPosition;
            break;

        case ES_String:
            buf = ((EjsString*) vp)->value;
            len = ((EjsString*) vp)->length;
            break;

        default:
            str = ejsToString(ejs, vp);
            buf = ejsGetString(str);
            len = str->length;
            break;
        }
        if (mprWrite(fp->file, buf, len) != len) {
            ejsThrowIOError(ejs, "Can't write to %s", fp->path);
            return 0;
        }
        written += len;
    }

    return (EjsVar*) ejsCreateNumber(ejs, written);
}



static int readData(Ejs *ejs, EjsFile *fp, EjsByteArray *ap, int offset, int count)
{
    char    buf[MPR_BUFSIZE];
    int     bufsize, totalRead, len, bytes;

    bufsize = ap->length - offset;

    for (totalRead = 0; count > 0; ) {
        len = min(count, bufsize);
        bytes = mprRead(fp->file, buf, bufsize);
        if (bytes < 0) {
            ejsThrowIOError(ejs, "Error reading from %s", fp->path);
        } else if (bytes == 0) {
            break;
        }
        mprMemcpy(&ap->value[offset], ap->length - offset, buf, bytes);
        count -= bytes;
        offset += bytes;
        totalRead += bytes;
    }
    return totalRead;
}


#if BLD_FEATURE_MMU && FUTURE
static void *mapFile(EjsFile *fp, uint size, int mode)
{
    Mpr         *mpr;
    void        *ptr;
    int x;

    mpr = mprGetMpr(fp);
    x = ~(mpr->alloc.pageSize - 1);
    size = (size + mpr->alloc.pageSize - 1) & ~(mpr->alloc.pageSize - 1);
#if MACOSX || LINUX || FREEBSD
    //  USE MAP_SHARED instead of MAP_PRIVATE if opened for write
    ptr = mmap(0, size, mode, MAP_FILE | MAP_PRIVATE, fp->file->fd, 0);
    x = errno;
#else
    ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, mapProt(mode));
#endif

    if (ptr == 0) {
        mprSetAllocError(mpr);
        return 0;
    }
    return ptr;
}


static void unmapFile(EjsFile *fp)
{
#if MACOSX || LINUX || FREEBSD
    munmap(fp->mapped, fp->info.size);
#else
    VirtualFree(file->mapped, 0, MEM_RELEASE);
#endif
}

#endif


EjsFile *ejsCreateFile(Ejs *ejs, cchar *path)
{
    EjsFile     *fp;
    EjsVar      *arg;

    fp = (EjsFile*) ejsCreateVar(ejs, ejsGetType(ejs, ES_ejs_io_File), 0);
    if (fp == 0) {
        return 0;
    }

    arg = (EjsVar*) ejsCreateString(ejs, path);
    fileConstructor(ejs, fp, 1, (EjsVar**) &arg);

    return fp;
}


int ejsCreateFileType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, "ejs.io", "File"), ejs->objectType, sizeof(EjsFile), ES_ejs_io_File,
        ES_ejs_io_File_NUM_CLASS_PROP, ES_ejs_io_File_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
    if (type == 0) {
        return EJS_ERR;
    }

    /*
     *  Define the helper functions.
     */
    type->helpers->getProperty = (EjsGetPropertyHelper) getFileProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setFileProperty;

    //  TODO - Need attribute for this */
    type->numericIndicies = 1;
    return 0;
}


int ejsConfigureFileType(Ejs *ejs)
{
    EjsType     *type;
    int         rc;

    type = ejsGetType(ejs, ES_ejs_io_File);

    rc = 0;
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_File, (EjsNativeFunction) fileConstructor);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_absolutePath, (EjsNativeFunction) absolutePath);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_basename, (EjsNativeFunction) basenameProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_close, (EjsNativeFunction) closeFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_copy, (EjsNativeFunction) copyFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_createTempFile, (EjsNativeFunction) createTempFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_created, (EjsNativeFunction) created);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_pathDelimiter, (EjsNativeFunction) pathDelimiter);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_pathDelimiter, (EjsNativeFunction) setPathDelimiter);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_dirname, (EjsNativeFunction) dirnameProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_exists, (EjsNativeFunction) exists);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_extension, (EjsNativeFunction) extension);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_freeSpace, (EjsNativeFunction) freeSpace);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_flush, (EjsNativeFunction) flushFile);
    rc += ejsBindMethod(ejs, type, ES_Object_get, (EjsNativeFunction) getFileIterator);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getBytes, (EjsNativeFunction) getBytes);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getLines, (EjsNativeFunction) getLines);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getString, (EjsNativeFunction) getFileAsString);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getXML, (EjsNativeFunction) getXML);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getFiles, (EjsNativeFunction) getFiles);
    rc += ejsBindMethod(ejs, type, ES_Object_getValues, (EjsNativeFunction) getValues);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_hasDriveSpec, (EjsNativeFunction) hasDriveSpec);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_isDir, (EjsNativeFunction) isDir);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_isOpen, (EjsNativeFunction) isOpen);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_isRegular, (EjsNativeFunction) isRegular);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_lastAccess, (EjsNativeFunction) lastAccess);
    rc += ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) fileLength);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_makeDir, (EjsNativeFunction) makeDir);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_mode, (EjsNativeFunction) mode);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_modified, (EjsNativeFunction) modified);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_name, (EjsNativeFunction) name);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_newline, (EjsNativeFunction) newline);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_newline, (EjsNativeFunction) setNewline);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_open, (EjsNativeFunction) openProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_parent, (EjsNativeFunction) parent);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_permissions, (EjsNativeFunction) getPermissions);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_permissions, (EjsNativeFunction) setPermissions);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_position, (EjsNativeFunction) getPosition);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_position, (EjsNativeFunction) setPosition);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_put, (EjsNativeFunction) putToFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_relativePath, (EjsNativeFunction) relativePath);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_readBytes, (EjsNativeFunction) readBytes);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_read, (EjsNativeFunction) readProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_remove, (EjsNativeFunction) removeFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_removeDir, (EjsNativeFunction) removeDir);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_rename, (EjsNativeFunction) renameProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_setCallback, (EjsNativeFunction) setFileCallback);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_unixPath, (EjsNativeFunction) unixPath);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_write, (EjsNativeFunction) fileWrite);

    return rc;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/io/ejsFile.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/io/ejsHttp.c"
 */
/************************************************************************/

/**
 *  ejsHttp.c - Http class. This implements a HTTP client.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if ES_ejs_io_Http && BLD_FEATURE_HTTP_CLIENT

static EjsVar   *getDateHeader(Ejs *ejs, EjsHttp *hp, cchar *key);
static EjsVar   *getStringHeader(Ejs *ejs, EjsHttp *hp, cchar *key);
static void     prepForm(Ejs *ejs, EjsHttp *hp, EjsVar *data);
static char     *prepUri(MprCtx ctx, char *uri);
static void     responseCallback(MprHttp *http, int nbytes);
static int      startRequest(Ejs *ejs, EjsHttp *hp, char *method, int argc, EjsVar **argv);

/*
 *  Constructor
 *
 *  function Http(uri: String = null)
 */
static EjsVar *httpConstructor(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    hp->ejs = ejs;
    hp->http = mprCreateHttp(hp);
    if (hp->http == 0) {
        ejsThrowMemoryError(ejs);
    }

    if (argc == 1 && argv[0] != ejs->nullValue) {
        hp->uri = prepUri(hp, ejsGetString(argv[0]));
    }
    hp->method = mprStrdup(hp, "GET");
    //  TODO - should have limit here
    hp->content = mprCreateBuf(hp, MPR_HTTP_BUFSIZE, -1);
    mprSetHttpCallback(hp->http, responseCallback, hp);

    return (EjsVar*) hp;
}


/*
 *  function addRequestHeader(key: String, value: String, overwrite: Boolean = true): Void
 */
EjsVar *addRequestHeader(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    char    *key, *value;
    bool    overwrite;

    mprAssert(argc >= 2);

    key = ejsGetString(argv[0]);
    value = ejsGetString(argv[1]);
    overwrite = (argc == 3) ? ejsGetBoolean(argv[2]) : 1;

    mprSetHttpHeader(hp->http, key, value, overwrite);
    return 0;
}


/*
 *  function get available(): Number
 */
EjsVar *httpAvailable(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetBufLength(hp->content));
}


/*
 *  function set callback(cb: Function): Void
 */
EjsVar *setHttpCallback(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1);

    hp->callback = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  function close(graceful: Boolean = true): Void
 */
static EjsVar *closeHttp(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->http) {
        mprDisconnectHttp(hp->http);
    }
    return 0;
}


/*
 *  function connect(): Void
 */
static EjsVar *connectHttp(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, NULL, argc, argv);
    return 0;
}


/**
 *  function get certificateFile(): String
 */
static EjsVar *getCertificateFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->certFile) {
        return (EjsVar*) ejsCreateString(ejs, hp->certFile);
    }
    return ejs->nullValue;
}


/*
 *  function set setCertificateFile(value: String): Void
 */
static EjsVar *setCertificateFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprFree(hp->certFile);
    hp->certFile = mprStrdup(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  function get code(): Number
 */
static EjsVar *code(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (!hp->gotResponse) {
        // throwHttpError(ejs, "Http has not received a response");
        return 0;
    }
    return (EjsVar*) ejsCreateNumber(ejs, mprGetHttpCode(hp->http));
}


/*
 *  function get codeString(): String
 */
static EjsVar *codeString(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (!hp->gotResponse) {
        // TODO throwHttpError(ejs, "Http has not received a response");
        return 0;
    }
    return (EjsVar*) ejsCreateString(ejs, mprGetHttpMessage(hp->http));
}


/*
 *  function get contentEncoding(): String
 */
static EjsVar *contentEncoding(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getStringHeader(ejs, hp, "CONTENT-ENCODING");
}


/*
 *  function get contentLength(): Number
 */
static EjsVar *contentLength(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    int     length;

    if (!hp->gotResponse) {
        // throwHttpError(ejs, "Http has not received a response");
        return 0;
    }
    length = mprGetHttpContentLength(hp->http);
    return (EjsVar*) ejsCreateNumber(ejs, length);
}


/*
 *  function get contentType(): String
 */
static EjsVar *contentType(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getStringHeader(ejs, hp, "CONTENT-TYPE");
}


/**
 *  function get date(): Date
 */
static EjsVar *date(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getDateHeader(ejs, hp, "DATE");
}


/*
 *  function del(uri: String = null): Void
 */
static EjsVar *del(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, "DELETE", argc, argv);
    return 0;
}


/**
 *  function get expires(): Date
 */
static EjsVar *expires(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getDateHeader(ejs, hp, "EXPIRES");
}


/*
 *  function form(uri: String = null, formData: Object = null): Void
 */
static EjsVar *form(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (argc == 2 && argv[1] != ejs->nullValue) {
        prepForm(ejs, hp, argv[1]);
    }
    startRequest(ejs, hp, "POST", argc, argv);
    return 0;
}


/*
 *
 *  static function get followRedirects(): Boolean
 */
static EjsVar *getFollowRedirects(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, hp->followRedirects);
}


/*
 *  function set followRedirects(flag: Boolean): Void
 */
static EjsVar *setFollowRedirects(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    hp->followRedirects = ejsGetBoolean(argv[0]);
    return 0;
}


/*
 *  function get(uri: String = null): Void
 */
static EjsVar *getHttpIterator(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, "GET", argc, argv);
    return 0;
}


/*
 *  function head(uri: String = null): Void
 */
static EjsVar *head(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, "HEAD", argc, argv);
    return 0;
}


/*
 *  function header(key: String): String
 */
static EjsVar *header(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetHttpHeader(hp->http, ejsGetString(argv[0])));
}


/*
 *  function get headers(): Object
 */
static EjsVar *headers(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    MprHashTable    *hash;
    MprHash         *p;
    EjsObject       *results;
    EjsName         qname;
    int             i;

    hash = mprGetHttpHeadersHash(hp->http);
    if (hash == 0) {
        return (EjsVar*) ejs->nullValue;
    }

    results = ejsCreateSimpleObject(ejs);

    for (i = 0, p = mprGetFirstHash(hash); p; p = mprGetNextHash(hash, p), i++) {
        ejsName(&qname, "", p->key);
        ejsSetPropertyByName(ejs, (EjsVar*) results, &qname, (EjsVar*) ejsCreateString(ejs, p->data));
    }
    return (EjsVar*) results;
}


/*
 *  function get isSecure(): Boolean
 */
static EjsVar *isSecure(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->http == 0 || hp->http->sock == 0) {
        return (EjsVar*) ejs->falseValue;
    }
    return (EjsVar*) ejsCreateBoolean(ejs, mprSocketIsSecure(hp->http->sock));
}


/*
 *  function get keyFile(): String
 */
static EjsVar *getKeyFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->keyFile) {
        return (EjsVar*) ejsCreateString(ejs, hp->keyFile);
    }
    return ejs->nullValue;
}


/*
 *  function set keyFile(keyFile: String): Void
 */
static EjsVar *setKeyFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprFree(hp->keyFile);
    hp->keyFile = mprStrdup(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  function get lastModified(): Date
 */
static EjsVar *lastModified(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getDateHeader(ejs, hp, "LAST-MODIFIED");
}


/*
 *  function get method(): String
 */
static EjsVar *getMethod(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, hp->method);
}


/*
 *  function set method(value: String): Void
 */
static EjsVar *setMethod(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    cchar    *method;

    method = ejsGetString(argv[0]);
    if (strcmp(method, "DELETE") != 0 && strcmp(method, "GET") != 0 &&  strcmp(method, "HEAD") != 0 &&
            strcmp(method, "OPTIONS") != 0 && strcmp(method, "POST") != 0 && strcmp(method, "PUT") != 0 &&
            strcmp(method, "TRACE") != 0) {
        ejsThrowArgError(ejs, "Unknown HTTP method");
        return 0;
    }
    mprFree(hp->method);
    hp->method = mprStrdup(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  function post(uri: String = null, postData: Array): Void
 */
static EjsVar *post(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsArray        *args;
    EjsNumber       *written;

    if (argc == 2 && ejsIsArray(argv[1])) {
        args = (EjsArray*) argv[1];
        if (args->length > 0) {
            data = ejsCreateByteArray(ejs, -1);
            written = ejsWriteToByteArray(ejs, data, 1, &argv[1]);
            hp->postData = (char*) data->value;
            hp->contentLength = (int) written->value;
        }
    }
    startRequest(ejs, hp, "POST", argc, argv);
    return 0;
}


/*
 *  function set postLength(length: Number): Void
 */
static EjsVar *setPostLength(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    hp->contentLength = (int) ejsGetNumber(argv[0]);
    return 0;
}



/*
 *  function put(uri: String = null, ...putData): Void
 */
static EjsVar *putToHttp(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsArray        *args;
    EjsNumber       *written;

    if (argc == 2 && argv[1] != ejs->nullValue) {
        args = (EjsArray*) argv[1];
        if (args->length > 0) {
            data = ejsCreateByteArray(ejs, -1);
            written = ejsWriteToByteArray(ejs, data, 1, &argv[1]);
            hp->postData = (char*) data->value;
            hp->contentLength = (int) written->value;
        }
    }
    startRequest(ejs, hp, "PUT", argc, argv);
    return 0;
}


/*
 *  function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
 */
static EjsVar *readHttpData(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *ba;
    int             offset, count, len, contentLength;

    count = -1;
    offset = -1;

    ba = (EjsByteArray*) argv[0];
    if (argc > 1) {
        offset = ejsGetInt(argv[1]);
    }
    if (argc > 2) {
        count = ejsGetInt(argv[2]);
    }
    if (count < 0) {
        count = MAXINT;
    }
    
    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    contentLength = mprGetBufLength(hp->content);
    len = min(contentLength - hp->readOffset, count);
    if (len > 0) {
        if (offset < 0) {
            ejsCopyToByteArray(ejs, ba, ba->writePosition, (char*) mprGetBufStart(hp->content), len);
            ejsSetByteArrayPositions(ejs, ba, -1, ba->writePosition + len);

        } else {
            ejsCopyToByteArray(ejs, ba, offset, (char*) mprGetBufEnd(hp->content), len);
        }
        mprAdjustBufStart(hp->content, len);
    }
    return (EjsVar*) ejsCreateNumber(ejs, len);
}


/*
 *  function readString(count: Number = -1): String
 *
 *  Read count bytes (default all) of content as a string. This always starts at the first character of content.
 */
static EjsVar *httpReadString(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsVar  *result;
    int     count;
    
    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    count = (argc == 1) ? ejsGetInt(argv[0]) : MAXINT;
    count = min(count, mprGetBufLength(hp->content));
    result = (EjsVar*) ejsCreateStringWithLength(ejs, mprGetBufStart(hp->content), count);

    mprAdjustBufStart(hp->content, count);
    
    return result;
}


/*
 *  function readLines(count: Number = -1): Array
 *
 *  Read count lines (default all) of content as a string. This always starts at the first line of content.
 */
static EjsVar *readLines(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsArray    *results;
    EjsVar      *str;
    cchar       *data, *cp, *last;
    int         count, nextIndex;

    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    count = (argc == 1) ? ejsGetInt(argv[0]) : MAXINT;
    results = ejsCreateArray(ejs, 0);

    if (mprGetBufLength(hp->content) > 0) {
        data = mprGetBufStart(hp->content);
        last = data;
        nextIndex = 0;

        for (cp = data; count > 0 && *cp; cp++) {
            if (*cp == '\r' || *cp == '\n') {
                if (last < cp) {
                    str = (EjsVar*) ejsCreateStringWithLength(ejs, last, (int) (cp - last));
                    ejsSetProperty(ejs, (EjsVar*) results, nextIndex++, str);
                }
                while (*cp == '\r' || *cp == '\n') {
                    cp++;
                }
                last = cp;
                if (*cp == '\0') {
                    cp--;
                }
                count--;
            }
        }
        if (last < cp) {
            str = (EjsVar*) ejsCreateStringWithLength(ejs, last, (int) (cp - last));
            ejsSetProperty(ejs, (EjsVar*) results, nextIndex++, str);
        }
        mprAdjustBufStart(hp->content, (int) (cp - data));
    }
    return (EjsVar*) results;
}


#if BLD_FEATURE_EJS_E4X
/*
 *  function readXml(): Stream
 */
static EjsVar *readXml(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsXML  *xml;

    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    xml = ejsCreateXML(ejs, 0, NULL, NULL, NULL);
    mprAddNullToBuf(hp->content);
    ejsLoadXMLString(ejs, xml, mprGetBufStart(hp->content));

    mprFlushBuf(hp->content);

    return (EjsVar*) xml;
}
#endif


/*
 *  function get responseStream(): Stream
 */
static EjsVar *responseStream(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }
    return (EjsVar*) hp;
}



/*
 *  function get requestStream(): Stream
 */
static EjsVar *requestStream(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function setCredentials(username: String, password: String): Void
 */
static EjsVar *setCredentials(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprSetHttpCredentials(hp->http, ejsGetString(argv[0]), ejsGetString(argv[1]));
    return 0;
}


/*
 *  function get timeout(): Number
 */
static EjsVar *getTimeout(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, hp->http->timeoutPeriod);
}



/*
 *  function set timeout(value: Number): Void
 */
static EjsVar *setTimeout(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprSetHttpTimeout(hp->http, (int) ejsGetNumber(argv[0]));
    return 0;
}



/*
 *  function get uri(): String
 */
static EjsVar *getUri(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, hp->uri);
}


/*
 *  function set uri(uri: String): Void
 */
static EjsVar *setUri(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprFree(hp->uri);
    hp->uri = prepUri(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  Write post data to the request stream. Connection must be in async mode by defining a callback.
 *
 *  function write(data: ByteArray): Void
 */
static EjsVar *httpWrite(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsNumber       *written;

    if (hp->callback == 0) {
        ejsThrowIOError(ejs, "Callback must be defined to use write");
        return 0;
    }

    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }
    mprAssert(hp->http->request);
    mprAssert(hp->http->sock);

    data = ejsCreateByteArray(ejs, -1);
    written = ejsWriteToByteArray(ejs, data, 1, &argv[0]);

    if (mprWriteHttpBody(hp->http, (char*) data->value, (int) written->value, 1) != (int) written->value) {
        ejsThrowIOError(ejs, "Can't write post data");
    }
    return 0;
}


/*
 *  Issue a request
 */
static int startRequest(Ejs *ejs, EjsHttp *hp, char *method, int argc, EjsVar **argv)
{
    int     flags;

    if (argc >= 1 && argv[0] != ejs->nullValue) {
        mprFree(hp->uri);
        hp->uri = prepUri(hp, ejsGetString(argv[0]));
    }
                                  
#if BLD_FEATURE_SSL
    if (strncmp(hp->uri, "https", 5) == 0) {
        if (!mprLoadSsl(ejs, 0)) {
            ejsThrowIOError(ejs, "Can't load SSL provider");
            return 0;
        }
    }
#endif
                                  
    if (method && strcmp(hp->method, method) != 0) {
        mprFree(hp->method);
        hp->method = mprStrdup(hp, method);
    }
    if (hp->method[0] != 'P' || hp->method[1] != 'O') {
        hp->contentLength = 0;
        hp->postData = 0;
    }

    mprFlushBuf(hp->content);

    hp->requestStarted = 1;
    hp->gotResponse = 0;


    if (hp->postData) {
        mprSetHttpBody(hp->http, hp->postData, hp->contentLength);
    }
    /*
     *  Block if a callback has been defined
     */
    flags = (hp->callback) ? MPR_HTTP_DONT_BLOCK : 0;
    if (mprHttpRequest(hp->http, hp->method, hp->uri, flags) < 0) {
        ejsThrowIOError(ejs, "Can't issue request for \"%s\"", hp->uri);
        return EJS_ERR;
    }
    return 0;
}


#if FUTURE
static EjsVar *getNumericHeader(Ejs *ejs, EjsHttp *hp, cchar *key)
{
    cchar   *value;

    if (!hp->gotResponse) {
        ejsThrowStateError(ejs, "Http has not received a response");
        return 0;
    }
    value = mprGetHttpHeader(hp->http, key);
    if (value == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) ejsCreateNumber(ejs, mprAtoi(value, 10));
}
#endif


static EjsVar *getDateHeader(Ejs *ejs, EjsHttp *hp, cchar *key)
{
    MprTime     when;
    cchar       *value;

    if (!hp->gotResponse) {
        ejsThrowStateError(ejs, "Http has not received a response");
        return 0;
    }
    value = mprGetHttpHeader(hp->http, key);
    if (value == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    if (mprParseTime(ejs, &when, value) < 0) {
        value = 0;
    }
    return (EjsVar*) ejsCreateDate(ejs, when);
}


static EjsVar *getStringHeader(Ejs *ejs, EjsHttp *hp, cchar *key)
{
    cchar       *value;

    if (!hp->gotResponse) {
        ejsThrowStateError(ejs, "Http has not received a response");
        return 0;
    }
    value = mprGetHttpHeader(hp->http, key);
    if (value == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) ejsCreateString(ejs, mprGetHttpHeader(hp->http, key));
}


/*
 *  Prepare form data as a series of key-value pairs. Data is formatted according to www-url-encoded specs by mprSetHttpFormData.
 *  E.g.  name=value&address=77%20Park%20Lane
 */
static void prepForm(Ejs *ejs, EjsHttp *hp, EjsVar *data)
{
    EjsName     qname;
    EjsVar      *vp;
    EjsString   *value;
    cchar       *key, *sep;
    char        *formData;
    int         i, count, len;

    len = 0;
    formData = 0;

    count = ejsGetPropertyCount(ejs, data);
    for (i = 0; i < count; i++) {
        qname = ejsGetPropertyName(ejs, data, i);
        key = qname.name;

        vp = ejsGetProperty(ejs, data, i);
        value = ejsToString(ejs, vp);

        sep = (formData) ? "&" : "";
        len = mprReallocStrcat(hp, &formData, -1, len, 0, sep, key, "=", value->value, 0);
    }
    len = (int) strlen(formData) * 3 + 1;
    hp->postData = mprAlloc(hp, len);
    mprUrlEncode(hp->postData, len, formData);
    hp->contentLength = (int) strlen(hp->postData);

    mprSetHttpHeader(hp->http, "Content-Type", "application/x-www-form-urlencoded", 1);
}


/*
 *  Called by MprHttp on response data and request failure or completion.
 */
static void responseCallback(MprHttp *http, int nbytes)
{
    Ejs         *ejs;
    EjsHttp     *hp;
    EjsObject   *event;
    EjsType     *eventType;
    EjsName     qname;
    EjsVar      *arg;

    hp = http->callbackArg;
    hp->gotResponse = 1;
    
    if (http->response && nbytes > 0) {
        mprResetBufIfEmpty(hp->content);        
        mprPutBlockToBuf(hp->content, mprGetBufStart(http->response->content), nbytes);
    }
    
    if (hp->callback) {
        ejs = hp->ejs;
        if (http->error) {
            /*
             *  Some kind of error
             */
            eventType = ejsGetType(ejs, ES_ejs_io_HttpErrorEvent);
            arg = (EjsVar*) ejsCreateString(ejs, http->error);
        } else {
            eventType = ejsGetType(ejs, ES_ejs_io_HttpDataEvent);
            arg = (EjsVar*) ejs->nullValue;
        }
        event = (EjsObject*) ejsCreateInstance(ejs, eventType, 1, (EjsVar**) &arg);
        if (event) {
            /*
             *  Invoked as:  callback(e: Event)  where e.data == http
             */
            ejsSetPropertyByName(ejs, (EjsVar*) event, ejsName(&qname, EJS_PUBLIC_NAMESPACE, "data"), (EjsVar*) hp);
            arg = (EjsVar*) event;
            ejsRunFunction(hp->ejs, hp->callback, 0, 1, &arg);
        }
    }
}


/*
 *  Prepare a URL by adding http:// as required
 */
static char *prepUri(MprCtx ctx, char *uri) 
{
    char    *newUri;

    if (*uri == '/') {
        mprAllocSprintf(ctx, &newUri, MPR_MAX_STRING, "http://127.0.0.1%s", uri);

    } else if (strstr(uri, "http://") == 0 && strstr(uri, "https://") == 0) {
        mprAllocSprintf(ctx, &newUri, MPR_MAX_STRING, "http://%s", uri);

    } else {
        newUri = mprStrdup(ctx, uri);
    }

    return newUri;
}
    


#if FUTURE
EjsHttp *ejsCreateHttp(Ejs *ejs)
{
    EjsHttp     *hp;

    hp = (EjsHttp*) ejsCreateVar(ejs, ejsGetType(ejs, ES_ejs_io_Http), 0);
    if (hp == 0) {
        return 0;
    }
    return hp;
}
#endif


int ejsCreateHttpType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, "ejs.io", "Http"), ejs->objectType, sizeof(EjsHttp), ES_ejs_io_Http,
        ES_ejs_io_Http_NUM_CLASS_PROP, ES_ejs_io_Http_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
    if (type == 0) {
        return EJS_ERR;
    }
    return 0;
}


int ejsConfigureHttpType(Ejs *ejs)
{
    EjsType     *type;
    int         rc;

    type = ejsGetType(ejs, ES_ejs_io_Http);

    rc = 0;
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_Http, (EjsNativeFunction) httpConstructor);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_addRequestHeader, (EjsNativeFunction) addRequestHeader);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_available, (EjsNativeFunction) httpAvailable);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_callback, (EjsNativeFunction) setHttpCallback);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_close, (EjsNativeFunction) closeHttp);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_connect, (EjsNativeFunction) connectHttp);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_certificateFile, (EjsNativeFunction) getCertificateFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_certificateFile, (EjsNativeFunction) setCertificateFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_code, (EjsNativeFunction) code);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_codeString, (EjsNativeFunction) codeString);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_contentEncoding, (EjsNativeFunction) contentEncoding);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_contentLength, (EjsNativeFunction) contentLength);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_contentType, (EjsNativeFunction) contentType);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_date, (EjsNativeFunction) date);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_del, (EjsNativeFunction) del);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_expires, (EjsNativeFunction) expires);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_followRedirects, (EjsNativeFunction) getFollowRedirects);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_followRedirects, (EjsNativeFunction) setFollowRedirects);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_form, (EjsNativeFunction) form);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_get, (EjsNativeFunction) getHttpIterator);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_head, (EjsNativeFunction) head);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_header, (EjsNativeFunction) header);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_headers, (EjsNativeFunction) headers);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_isSecure, (EjsNativeFunction) isSecure);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_keyFile, (EjsNativeFunction) getKeyFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_keyFile, (EjsNativeFunction) setKeyFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_lastModified, (EjsNativeFunction) lastModified);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_method, (EjsNativeFunction) getMethod);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_method, (EjsNativeFunction) setMethod);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_post, (EjsNativeFunction) post);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_postLength, (EjsNativeFunction) setPostLength);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_put, (EjsNativeFunction) putToHttp);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_read, (EjsNativeFunction) readHttpData);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_readString, (EjsNativeFunction) httpReadString);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_readLines, (EjsNativeFunction) readLines);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_requestStream, (EjsNativeFunction) requestStream);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_responseStream, (EjsNativeFunction) responseStream);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_setCredentials, (EjsNativeFunction) setCredentials);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_timeout, (EjsNativeFunction) getTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_timeout, (EjsNativeFunction) setTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_uri, (EjsNativeFunction) getUri);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_uri, (EjsNativeFunction) setUri);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_write, (EjsNativeFunction) httpWrite);

#if BLD_FEATURE_EJS_E4X
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_readXml, (EjsNativeFunction) readXml);
#endif
    return rc;
}


#endif /* ES_Http */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/io/ejsHttp.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/io/ejsSocket.c"
 */
/************************************************************************/

/**
 *  ejsSocket.c - Socket class. This implements TCP/IP v4 and v6 connectivity.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/** THIS CODE IS NOT IMPLEMENTED - just cut and pasted from HTTP */
#if ES_ejs_io_Socket && 0

static EjsVar   *getDateHeader(Ejs *ejs, EjsSocket *sp, cchar *key);
static EjsVar   *getStringHeader(Ejs *ejs, EjsSocket *sp, cchar *key);
static void     prepFormData(Ejs *ejs, EjsSocket *sp, EjsVar *data);
static char     *prepUrl(MprCtx ctx, char *url);
static void     responseCallback(MprSocket *socket, int nbytes);
static int      startRequest(Ejs *ejs, EjsSocket *sp, char *method, int argc, EjsVar **argv);

/*
 *  Constructor
 *
 *  function Socket(url: String = null)
 */
static EjsVar *socketConstructor(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    sp->ejs = ejs;
    sp->socket = mprCreateSocket(sp);
    if (sp->socket == 0) {
        ejsThrowMemoryError(ejs);
    }

    if (argc == 1 && argv[0] != ejs->nullValue) {
        sp->url = prepUrl(sp, ejsGetString(argv[0]));
    }
    sp->method = mprStrdup(sp, "GET");
    //  TODO - should have limit here
    sp->content = mprCreateBuf(sp, MPR_SOCKET_BUFSIZE, -1);
    mprSetSocketCallback(sp->socket, responseCallback, sp);

    return (EjsVar*) sp;
}


/*
 *  function get address(): String
 */
EjsVar *address(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get available(): Number
 */
EjsVar *socketAvailable(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetBufLength(sp->content));
}


/*
 *  function set callback(cb: Function): Void
 */
EjsVar *setSocketCallback(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1);

    sp->callback = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  function close(graceful: Boolean = true): Void
 */
static EjsVar *closeSocket(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    if (sp->socket) {
        mprDisconnectSocket(sp->socket);
    }
    return 0;
}


/*
 *  function connect(): Void
 */
static EjsVar *connectSocket(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    startRequest(ejs, sp, NULL, argc, argv);
    return 0;
}


/**
 *  function get eof(): Boolean
 */
static EjsVar *eof(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function flush(): Void
 */
static EjsVar *flushProc(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/**
 *  function listen(address: String = "", port: Number = 0): Socket
 */
static EjsVar *listenProc(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get mode(): Number
 */
static EjsVar *getMode(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function set mode(value: Number): Void
 */
static EjsVar *setMode(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get port(): Number
 */
static EjsVar *port(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
 */
static EjsVar *readProc(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    EjsByteArray    *ba;
    int             offset, count, len, contentLength;

    count = -1;
    offset = -1;

    ba = (EjsByteArray*) argv[0];
    if (argc > 1) {
        offset = ejsGetInt(argv[1]);
    }
    if (argc > 2) {
        count = ejsGetInt(argv[2]);
    }
    if (count < 0) {
        count = MAXINT;
    }
    
    if (!sp->requestStarted && startRequest(ejs, sp, NULL, 0, NULL) < 0) {
        return 0;
    }

    contentLength = mprGetBufLength(sp->content);
    len = min(contentLength - sp->readOffset, count);
    if (len > 0) {
        if (offset < 0) {
            ejsCopyToByteArray(ejs, ba, ba->writePosition, (char*) mprGetBufStart(sp->content), len);
            ejsSetByteArrayPositions(ba, -1, ba->writePosition + len);

        } else {
            ejsCopyToByteArray(ejs, ba, offset, (char*) mprGetBufEnd(sp->content), len);
        }
        mprAdjustBufStart(sp->content, len);
    }
    return (EjsVar*) ejsCreateNumber(ejs, len);
}


/*
 *  function get remoteAddress(): String
 */
static EjsVar *remoteAddress(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get room(): Number
 */
static EjsVar *room(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}



/*
 *  function set timeout(value: Number): Void
 */
static EjsVar *timeout(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}



/*
 *  function inputStream(): Stream
 */
static EjsVar *requestStream(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function outputStream(): Stream
 */
static EjsVar *requestStream(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  Write post data to the request stream. Connection must be in async mode by defining a callback.
 *
 *  function write(data: ByteArray): Void
 */
static EjsVar *socketWrite(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsNumber       *written;

    if (sp->callback == 0) {
        ejsThrowIOError(ejs, "Callback must be defined to use write");
        return 0;
    }

    if (!sp->requestStarted && startRequest(ejs, sp, NULL, 0, NULL) < 0) {
        return 0;
    }
    mprAssert(sp->socket->request);
    mprAssert(sp->socket->sock);

    data = ejsCreateByteArray(ejs, -1);
    written = ejsWriteToByteArray(ejs, data, 1, &argv[0]);

    if (mprWriteSocketPostData(sp->socket, (char*) data->value, written->value, 1) != written->value) {
        ejsThrowIOError(ejs, "Can't write post data");
    }
    return 0;
}


/*
 *  Issue a request
 */
static int startRequest(Ejs *ejs, EjsSocket *sp, char *method, int argc, EjsVar **argv)
{
    int     flags;

    if (argc >= 1 && argv[0] != ejs->nullValue) {
        mprFree(sp->url);
        sp->url = prepUrl(sp, ejsGetString(argv[0]));
    }
                                  
#if BLD_FEATURE_SSL
    if (strncmp(sp->url, "sockets", 5) == 0) {
        if (mprInitSSL(ejs, NULL, NULL, NULL, NULL, NULL, 0) < 0) {
            ejsThrowIOError(ejs, "Can't load SSL provider");
            return 0;
        }
    }
#endif
                                  
    if (method && strcmp(sp->method, method) != 0) {
        mprFree(sp->method);
        sp->method = mprStrdup(sp, method);
    }
    if (sp->method[0] != 'P' || sp->method[1] != 'O') {
        sp->contentLength = 0;
        sp->postData = 0;
    }

    mprFlushBuf(sp->content);

    sp->requestStarted = 1;
    sp->gotResponse = 0;

    /*
     *  Block if a callback has been defined
     */
    flags = (sp->callback) ? MPR_SOCKET_DONT_BLOCK : 0;
    if (mprSocketRequest(sp->socket, sp->method, sp->url, sp->postData, sp->contentLength, flags) < 0) {
        ejsThrowIOError(ejs, "Can't issue request for \"%s\"", sp->url);
        return EJS_ERR;
    }
    return 0;
}



/*
 *  Called by MprSocket on response data and request failure or completion.
 */
static void responseCallback(MprSocket *socket, int nbytes)
{
    Ejs         *ejs;
    EjsSocket     *hp;
    EjsObject   *event;
    EjsType     *eventType;
    EjsName     qname;
    EjsVar      *arg;

    hp = socket->callbackArg;
    sp->gotResponse = 1;
    
    if (socket->response && nbytes > 0) {
        mprResetBufIfEmpty(sp->content);        
        mprPutBlockToBuf(sp->content, mprGetBufStart(socket->response->content), nbytes);
    }
    
    if (sp->callback) {
        ejs = sp->ejs;
        if (socket->error) {
            /*
             *  Some kind of error
             */
            eventType = ejsGetType(ejs, ES_ejs_io_SocketError);
            arg = (EjsVar*) ejsCreateString(ejs, socket->error);
        } else {
            eventType = ejsGetType(ejs, ES_ejs_io_SocketDataEvent);
            arg = (EjsVar*) ejs->nullValue;
        }
        event = (EjsObject*) ejsCreateInstance(ejs, eventType, 1, (EjsVar**) &arg);
        if (event) {
            /*
             *  Invoked as:  callback(e: Event)  where e.data == socket
             */
            ejsSetPropertyByName(ejs, (EjsVar*) event, ejsName(&qname, EJS_PUBLIC_NAMESPACE, "data"), (EjsVar*) sp);
            arg = (EjsVar*) event;
            ejsRunFunction(sp->ejs, sp->callback, 0, 1, &arg);
        }
    }
}



EjsSocket *ejsCreateSocket(Ejs *ejs)
{
    EjsSocket     *hp;

    hp = (EjsSocket*) ejsCreateVar(ejs, ejsGetType(ejs, ES_ejs_io_Socket), 0);
    if (hp == 0) {
        return 0;
    }
    return hp;
}


int ejsCreateSocketType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    //  TODO - attributes
    type = ejsCreateCoreType(ejs, ejsName(&qname, "ejs.io", "Socket"), ejs->objectType, sizeof(EjsSocket), 
        ES_ejs_io_Socket, ES_ejs_io_Socket_NUM_CLASS_PROP, ES_ejs_io_Socket_NUM_INSTANCE_PROP);
        EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
    if (type == 0) {
        return EJS_ERR;
    }
    return 0;
}


int ejsConfigureSocketType(Ejs *ejs)
{
    EjsType     *type;
    int         rc;

    type = ejsGetType(ejs, ES_ejs_io_Socket);

    rc = 0;
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_Socket, (EjsNativeFunction) socketConstructor);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_address, (EjsNativeFunction) address);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_available, (EjsNativeFunction) socketAvailable, -1, 0);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_set_callback, (EjsNativeFunction) setSocketCallback);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_close, (EjsNativeFunction) closeSocket);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_connect, (EjsNativeFunction) connectSocket);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_eof, (EjsNativeFunction) eof);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_flush, (EjsNativeFunction) flush);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_listen, (EjsNativeFunction) listen);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_mode, (EjsNativeFunction) getMode);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_set_mode, (EjsNativeFunction) getMode);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_port, (EjsNativeFunction) port);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_read, (EjsNativeFunction) readProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_inputStream, (EjsNativeFunction) inputStream, -1, 0);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_outputStream, (EjsNativeFunction) outputStream, -1, 0);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_remoteAddress, (EjsNativeFunction) remoteAddress);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_room, (EjsNativeFunction) room);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_timeout, (EjsNativeFunction) getTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_set_timeout, (EjsNativeFunction) setTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_write, (EjsNativeFunction) socketWrite);

    return rc;
}


#else /* ES_Socket */
void __dummySocket() {}
#endif /* ES_Socket */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/io/ejsSocket.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsApp.c"
 */
/************************************************************************/

/*
 *  ejsApp.c -- App class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Get the application command line arguments
 *
 *  static function get args(): String
 */
static EjsVar *getArgs(Ejs *ejs, EjsObject *unused, int argc, EjsVar **argv)
{
    EjsArray    *args;
    int         i;

    args = ejsCreateArray(ejs, ejs->argc);
    for (i = 0; i < ejs->argc; i++) {
        ejsSetProperty(ejs, (EjsVar*) args, i, (EjsVar*) ejsCreateString(ejs, ejs->argv[i]));
    }
    return (EjsVar*) args;
}


/*
 *  Get the application startup directory
 *
 *  static function get dir(): String
 */
static EjsVar *getDir(Ejs *ejs, EjsObject *unused, int argc, EjsVar **argv)
{
    char        path[MPR_MAX_FNAME];

    if (mprGetAppDir(ejs, path, sizeof(path)) == 0) {
        ejsThrowIOError(ejs, "Can't get application directory");
        return 0;
    }

    return (EjsVar*) ejsCreateString(ejs, path);
}


/**
 *  Exit the application
 *
 *  static function exit(status: Number): void
 */
static EjsVar *exitMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    int     status;

    status = argc == 0 ? 0 : ejsGetInt(argv[0]);
    if (status != 0) {
        exit(status);
    } else {
        mprTerminate(mprGetMpr(ejs), 1);
    }
    return 0;
}


/**
 *  Control if the application will exit when the last script completes.
 *
 *  static function noexit(exit: Boolean): void
 */
static EjsVar *noexit(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    ejs->flags |= EJS_FLAG_NOEXIT;
    return 0;
}


/**
 *  Service events
 *
 *  static function serviceEvents(count: Number = -1, timeout: Number = -1): void
 */
static EjsVar *serviceEvents(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprTime     start;
    int         count, timeout;

    count = (argc > 1) ? ejsGetInt(argv[0]) : MAXINT;
    timeout = (argc > 1) ? ejsGetInt(argv[1]) : MAXINT;
    if (count < 0) {
        count = MAXINT;
    }
    if (timeout < 0) {
        timeout = MAXINT;
    }

    start = mprGetTime(ejs);
    do {
        mprServiceEvents(ejs, timeout, MPR_SERVICE_ONE_THING);
        timeout -= (int) (mprGetTime(ejs) - start);
        count--;
    } while (count > 0 && timeout > 0);

    return 0;
}


/**
 *  Pause the application
 *
 *  static function sleep(delay: Number = -1): void
 */
static EjsVar *sleepProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprTime     start;
    int         delay;

    delay = (argc > 0) ? ejsGetInt(argv[0]): MAXINT;
    if (delay < 0) {
        delay = MAXINT;
    }

    start = mprGetTime(ejs);
    do {
        mprServiceEvents(ejs, delay, 0);
        delay -= (int) (mprGetTime(ejs) - start);
    } while (delay > 0);

    return 0;
}


static EjsVar *workingDir(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    char    path[MPR_MAX_FNAME];

    getcwd(path, sizeof(path));
#if BLD_WIN_LIKE
    mprMapDelimiters(ejs, path, '/');
#endif
    return (EjsVar*) ejsCreateString(ejs, path);
}


static EjsVar *setWorkingDir(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    char    *path;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    path = ejsGetString(argv[0]);

    if (chdir(path) < 0) {
        ejsThrowIOError(ejs, "Can't change the working directory");
    }
    return 0;
}


void ejsCreateAppType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "App"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_App,
        ES_ejs_sys_App_NUM_CLASS_PROP, ES_ejs_sys_App_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureAppType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_App);

    ejsBindMethod(ejs, type, ES_ejs_sys_App_args, (EjsNativeFunction) getArgs);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_dir, (EjsNativeFunction) getDir);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_exit, (EjsNativeFunction) exitMethod);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_noexit, (EjsNativeFunction) noexit);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_serviceEvents, (EjsNativeFunction) serviceEvents);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_sleep, (EjsNativeFunction) sleepProc);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_workingDir, (EjsNativeFunction) workingDir);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_set_workingDir, (EjsNativeFunction) setWorkingDir);

#if FUTURE
    (ejs, type, ES_ejs_sys_App_permissions, (EjsNativeFunction) getPermissions,
        ES_ejs_sys_App_set_permissions, (EjsNativeFunction) setPermissions);
#endif
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsApp.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsConfig.c"
 */
/************************************************************************/

/*
 *  ejsConfig.c -- Config class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




void ejsCreateConfigType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "Config"), ejs->objectType, sizeof(EjsObject), 
        ES_Config, ES_ejs_sys_Config_NUM_CLASS_PROP, ES_ejs_sys_Config_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureConfigType(Ejs *ejs)
{
    EjsVar      *vp;
    char        version[16];

    vp = (EjsVar*) ejsGetType(ejs, ES_Config);
    if (vp == 0) {
        return;
    }
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Debug, BLD_DEBUG ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_CPU, (EjsVar*) ejsCreateString(ejs, BLD_HOST_CPU));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_DB, BLD_FEATURE_EJS_DB ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_E4X, BLD_FEATURE_EJS_E4X ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Floating, 
        BLD_FEATURE_FLOATING_POINT ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Http, BLD_FEATURE_HTTP ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);

    if (BLD_FEATURE_EJS_LANG == EJS_SPEC_ECMA) {
        ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Lang, (EjsVar*) ejsCreateString(ejs, "ecma"));
    } else if (BLD_FEATURE_EJS_LANG == EJS_SPEC_PLUS) {
        ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Lang, (EjsVar*) ejsCreateString(ejs, "plus"));
    } else {
        ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Lang, (EjsVar*) ejsCreateString(ejs, "fixed"));
    }

    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Legacy, 
        BLD_FEATURE_LEGACY_API ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Multithread, 
        BLD_FEATURE_MULTITHREAD ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);

    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_NumberType, 
        (EjsVar*) ejsCreateString(ejs, MPR_STRINGIFY(BLD_FEATURE_NUM_TYPE)));

    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_OS, (EjsVar*) ejsCreateString(ejs, BLD_OS));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Product, (EjsVar*) ejsCreateString(ejs, BLD_PRODUCT));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_RegularExpressions, 
        BLD_FEATURE_REGEXP ? (EjsVar*) ejs->trueValue: (EjsVar*) ejs->falseValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Title, (EjsVar*) ejsCreateString(ejs, BLD_NAME));

    mprSprintf(version, sizeof(version), "%s-%s", BLD_VERSION, BLD_NUMBER);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Version, (EjsVar*) ejsCreateString(ejs, version));

#if BLD_WIN_LIKE
{
	/*
	 *	Users may install Ejscript in a different location
	 */
	char	path[MPR_MAX_FNAME], dir[MPR_MAX_FNAME];
	mprGetAppPath(ejs, path, sizeof(path));
	mprGetDirName(dir, sizeof(dir), path);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_BinDir, (EjsVar*) ejsCreateString(ejs, dir));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_LibDir, (EjsVar*) ejsCreateString(ejs, dir));
}
#else
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_BinDir, (EjsVar*) ejsCreateString(ejs, BLD_BIN_PREFIX));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_LibDir, (EjsVar*) ejsCreateString(ejs, BLD_LIB_PREFIX));
#endif
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsConfig.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsDebug.c"
 */
/************************************************************************/

/*
 *  ejsDebug.c - System.Debug class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#if UNUSED
/*
 *  function bool isDebugMode()
 *  MOB -- convert to accessor
 */

static int isDebugMode(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    ejsTrace(fp, "isDebugMode()\n");
    ejsSetReturnValueToInteger(fp, mprGetDebugMode(fp));
    return 0;
}


int ejsDefineDebugClass(EjsFiber *fp)
{
    EjsVar  *systemDebugClass;

    systemDebugClass =  ejsDefineClass(fp, "System.Debug", "Object", 0);
    if (systemDebugClass == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }

    /*
     *  Define the class methods
     */
    ejsDefineCMethod(fp, systemDebugClass, "isDebugMode", isDebugMode,
        0);

    return ejsObjHasErrors(systemDebugClass) ? MPR_ERR_CANT_INITIALIZE : 0;
}
#else
void __dummyEjsDebug() {}
#endif

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsDebug.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsGC.c"
 */
/************************************************************************/

/**
 *  ejsGC.c - Garbage collector class for the EJS Object Model
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  native static function get allocatedMemory(): Number
 */
static EjsVar *getAllocatedMemory(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    MprAlloc    *alloc;

    alloc = mprGetAllocStats(ejs);
    return (EjsVar*) ejsCreateNumber(ejs, alloc->bytesAllocated);
}


/*
 *  native static function get enabled(): Boolean
 */
static EjsVar *getEnable(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    return (EjsVar*) ((ejs->gc.enabled) ? ejs->trueValue: ejs->falseValue);
}


/*
 *  native static function set enabled(on: Boolean): Void
 */
static EjsVar *setEnable(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsBoolean(argv[0]));
    ejs->gc.enabled = ejsGetBoolean(argv[0]);
    return 0;
}


/*
 *  native static function get maxMemory(): Number
 */
static EjsVar *getMaxMemory(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    MprAlloc    *alloc;

    alloc = mprGetAllocStats(ejs);
    return (EjsVar*) ejsCreateNumber(ejs, alloc->maxMemory);
}


/*
 *  native static function set maxMemory(limit: Number): Void
 */
static EjsVar *setMaxMemory(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    int     maxMemory, redLine;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    maxMemory = ejsGetInt(argv[0]);

    /*
     *  Set the redline at 95% of the maximum
     */
    redLine = maxMemory / 20 * 19;
    mprSetAllocLimits(ejs, redLine, maxMemory);
    return 0;
}


/*
 *  native static function get peakMemory(): Number
 */
static EjsVar *getPeakMemory(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    MprAlloc    *alloc;

    alloc = mprGetAllocStats(ejs);
    return (EjsVar*) ejsCreateNumber(ejs, alloc->peakAllocated);
}


/*
 *  native static function printStats(): Void
 */
static EjsVar *printGCStats(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    ejsPrintAllocReport(ejs);
    mprPrintAllocReport(ejs, "Memroy Report");
    return 0;
}


/*
 *  run(deep: Boolean = false)
 */
static EjsVar *runGC(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    int     deep;

    deep = ((argc == 1) && ejsIsBoolean(argv[1]));

    if (deep) {
        ejsCollectGarbage(ejs, EJS_GC_ALL);
    } else { 
        ejsCollectGarbage(ejs, EJS_GC_SMART);
    }
    return 0;
}


/*
 *  native static function get workQuota(): Number
 */
static EjsVar *getWorkQuota(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ejs->gc.workQuota);
}


/*
 *  native static function set workQuota(quota: Number): Void
 */
static EjsVar *setWorkQuota(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
    int     quota;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    quota = ejsGetInt(argv[0]);

    if (quota < EJS_GC_MIN_WORK_QUOTA && quota != 0) {
        ejsThrowArgError(ejs, "Bad work quota");
        return 0;
    }
    ejs->gc.workQuota = quota;
    return 0;
}



void ejsCreateGCType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "GC"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_GC,
        ES_ejs_sys_GC_NUM_CLASS_PROP, ES_ejs_sys_GC_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureGCType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_GC);

    ejsBindMethod(ejs, type, ES_ejs_sys_GC_allocatedMemory, (EjsNativeFunction) getAllocatedMemory);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_enabled, (EjsNativeFunction) getEnable);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_set_enabled, (EjsNativeFunction) setEnable);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_maxMemory, (EjsNativeFunction) getMaxMemory);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_set_maxMemory, (EjsNativeFunction) setMaxMemory);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_peakMemory, (EjsNativeFunction) getPeakMemory);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_workQuota, (EjsNativeFunction) getWorkQuota);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_set_workQuota, (EjsNativeFunction) setWorkQuota);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_printStats, (EjsNativeFunction) printGCStats);
    ejsBindMethod(ejs, type, ES_ejs_sys_GC_run, (EjsNativeFunction) runGC);
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsGC.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsLogger.c"
 */
/************************************************************************/

/*
 *  ejsLogger.c - Logger class 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#if UNUSED
/*
 *  System.Log.setLog(path);
 *  System.Log.enable;
 */

static void logHandler(MPR_LOC_DEC(ctx, loc), int flags, int level, 
    const char *msg)
{
    Mpr *app;
    char    *buf;
    int     len;

    app = mprGetApp(ctx);
    if (app->logFile == 0) {
        return;
    }

    if (flags & MPR_LOG_SRC) {
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Log %d: %s\n", level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Error: %s\n", msg);

    } else if (flags & MPR_FATAL_SRC) {
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Fatal: %s\n", msg);
        
    } else if (flags & MPR_ASSERT_SRC) {
#if BLD_FEATURE_ALLOC_LEAK_TRACK
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Assertion %s, failed at %s\n",
            msg, loc);
#else
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Assertion %s, failed\n", msg);
#endif

    } else if (flags & MPR_RAW) {
        /* OPT */
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "%s", msg);

    } else {
        return;
    }

    mprPuts(app->logFile, buf, len);

    mprFree(buf);
}

/*
 *  function int setLog(string path)
 */

static int setLog(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    const char  *path;
    MprFile     *file;
    Mpr     *app;

    if (argc != 1 || !ejsVarIsString(argv[0])) {
        ejsArgError(fp, "Usage: setLog(path)");
        return -1;
    }

    app = mprGetApp(fp);

    /*
     *  Ignore errors if we can't create the log file.
     *  Use the app context so this will live longer than the interpreter
     *  MOB -- this leaks files.
     */
    path = argv[0]->string;
    file = mprOpen(app, path, O_CREAT | O_TRUNC | O_WRONLY, 0664);
    if (file) {
        app->logFile = file;
        mprSetLogHandler(fp, logHandler);
    }
    mprLog(fp, 0, "Test log");

    return 0;
}

#if UNUSED

static int enableSetAccessor(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    if (argc != 1) {
        ejsArgError(fp, "Usage: set(value)");
        return -1;
    }
    ejsSetProperty(fp, thisObj, "_enabled", argv[0]);
    return 0;
}


static int enableGetAccessor(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    ejsSetReturnValue(fp, ejsGetPropertyAsVar(fp, thisObj, "_enabled"));
    return 0;
}

#endif

int ejsDefineLogClass(EjsFiber *fp)
{
    EjsVar          *logClass;

    logClass =  ejsDefineClass(fp, "System.Log", "Object", 0);
    if (logClass == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }

    ejsDefineCMethod(fp, logClass, "setLog", setLog, 0);

#if UNUSED
    EjsProperty     *pp;
    ejsDefineCAccessors(fp, logClass, "enable", enableSetAccessor, 
        enableGetAccessor, 0);

    pp = ejsSetPropertyToBoolean(fp, logClass, "_enabled", 0);
    ejsMakePropertyEnumerable(pp, 0);
#endif

    return ejsObjHasErrors(logClass) ? MPR_ERR_CANT_INITIALIZE : 0;
}

#else
void __dummyEjsLog() {}
#endif

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsLogger.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsMemory.c"
 */
/************************************************************************/

/*
 *  ejsMemory.c - Memory class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if FUTURE
static EjsVar *getUsedMemoryProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, getUsedMemory(ejs));
}


static int getUsedStackProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprStackSize(ejs));
}


uint ejsGetAvailableMemory(Ejs *ejs)
{
    EjsVar          *memoryClass;
    uint            ram;

    memoryClass =  ejsFindClass(ejs, 0, "System.Memory");

    ram = ejsGetPropertyAsInteger(ejs, memoryClass, "ram");
    return ram - getUsedMemory(ejs);
}



static EjsVar *getAvailableMemoryProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsVar          *memoryClass;
    uint            ram;

    memoryClass = ejsFindClass(ejs, 0, "System.Memory");

    ram = ejsGetPropertyAsInteger(ejs, memoryClass, "ram");
    ejsSetReturnValueToInteger(ejs, 0);
    return 0;
}



static uint getUsedMemory(Ejs *ejs)
{
    return 0;
}
#endif


static EjsVar *printStats(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    mprPrintAllocReport(ejs, "Memory.printStats()");
    ejsPrintAllocReport(ejs);
    return 0;
}


void ejsCreateMemoryType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "Memory"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_Memory,
        ES_ejs_sys_Memory_NUM_CLASS_PROP, ES_ejs_sys_Memory_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureMemoryType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_Memory);

    ejsBindMethod(ejs, type, ES_ejs_sys_Memory_printStats, (EjsNativeFunction) printStats);
#if FUTURE
    ejsBindMethod(ejs, type, ES_ejs_sys_App_NAME, (EjsNativeFunction) NAME);
#endif
}

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsMemory.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/sys/ejsSystem.c"
 */
/************************************************************************/

/*
 *  ejsSystem.c -- System class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_CMD
#if ES_ejs_sys_System_run
/*
 *  function run(cmd: String): String
 */
static EjsVar *run(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprCmd      *cmd;
    EjsString   *result;
    char        *err, *output;
    int         status;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    cmd = mprCreateCmd(ejs);
    status = mprRunCmd(cmd, ejsGetString(argv[0]), &output, &err, 0);
    if (status) {
        ejsThrowError(ejs, "Command failed: %s, status %d", err, status);
        mprFree(cmd);
        return 0;
    }
    result = ejsCreateString(ejs, output);
    mprFree(cmd);
    return (EjsVar*) result;
}
#endif


//  TODO - refactor and rename
#if ES_ejs_sys_System_runx
/*
 *  function runx(cmd: String): Void
 */
static EjsVar *runx(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprCmd      *cmd;
    char        *err;
    int         status;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    cmd = mprCreateCmd(ejs);
    status = mprRunCmd(cmd, ejsGetString(argv[0]), NULL, &err, 0);
    if (status) {
        ejsThrowError(ejs, "Can't run command: %s\nDetails: %s", ejsGetString(argv[0]), err);
        mprFree(err);
    }
    mprFree(cmd);
    return 0;
}
#endif
#endif


void ejsCreateSystemType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "System"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_System,
        ES_ejs_sys_System_NUM_CLASS_PROP, ES_ejs_sys_System_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureSystemType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_System);

#if BLD_FEATURE_CMD
#if ES_ejs_sys_System_run
    ejsBindMethod(ejs, type, ES_ejs_sys_System_run, (EjsNativeFunction) run);
#endif
#if ES_ejs_sys_System_runx
    ejsBindMethod(ejs, type, ES_ejs_sys_System_runx, (EjsNativeFunction) runx);
#endif
#endif
}



/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/sys/ejsSystem.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/xml/ejsXML.c"
 */
/************************************************************************/

/**
 *  ejsXML.c - E4X XML type.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_E4X

/*
 *  XML methods
 */
static EjsVar   *loadXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv);
static EjsVar   *saveXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv);

#if UNUSED
static EjsVar   *toString(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *valueOf(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *toXmlString(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *appendChild(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *attributes(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *child(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *elements(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *comments(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *decendants(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *elements(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *insertChildAfter(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *insertChildBefore(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *replace(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *setName(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *text(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);

#endif

static bool allDigitsForXml(cchar *name);
static bool deepCompare(EjsXML *lhs, EjsXML *rhs);
static int readStringData(MprXml *xp, void *data, char *buf, int size);
static int readFileData(MprXml *xp, void *data, char *buf, int size);


static EjsXML *createXml(Ejs *ejs, EjsType *type, int size)
{
    return (EjsXML*) ejsCreateXML(ejs, 0, NULL, NULL, NULL);
}


static void destroyXml(Ejs *ejs, EjsXML *xml)
{
    ejsFreeVar(ejs, (EjsVar*) xml);
}


static EjsVar *cloneXml(Ejs *ejs, EjsXML *xml, bool deep)
{
    EjsXML  *newXML;

    //  TODO - implement deep copy.

    newXML = (EjsXML*) ejsCreateVar(ejs, xml->var.type, 0);
    if (newXML == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    //  TODO complete
    return (EjsVar*) newXML;
}


/*
 *  Cast the object operand to a primitive type
 */
static EjsVar *castXml(Ejs *ejs, EjsXML *xml, EjsType *type)
{
    EjsXML      *item;
    EjsVar      *result;
    MprBuf      *buf;

    mprAssert(ejsIsXML(xml));

    switch (type->id) {
    case ES_Object:
    case ES_XMLList:
        return (EjsVar*) xml;

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_Number:
        result = castXml(ejs, xml, ejs->stringType);
        result = (EjsVar*) ejsToNumber(ejs, result);
        return result;

    case ES_String:
        if (xml->kind == EJS_XML_ELEMENT) {
            if (xml->elements == 0) {
                return (EjsVar*) ejs->emptyStringValue;
            }
            if (xml->elements && mprGetListCount(xml->elements) == 1) {
                //  TODO - what about PI and comments?
                item = mprGetFirstItem(xml->elements);
                if (item->kind == EJS_XML_TEXT) {
                    return (EjsVar*) ejsCreateString(ejs, item->value);
                }
            }
        }
        buf = mprCreateBuf(ejs, MPR_BUFSIZE, -1);
        if (ejsXMLToString(ejs, buf, xml, -1) < 0) {
            mprFree(buf);
            return 0;
        }
        result = (EjsVar*) ejsCreateString(ejs, (char*) buf->start);
        mprFree(buf);
        return result;

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
    return 0;
}


static int deleteXmlPropertyByName(Ejs *ejs, EjsXML *xml, EjsName *qname)
{
    EjsXML      *item;
    bool        removed;
    int         next;

    removed = 0;

    if (qname->name[0] == '@') {
        /* @ and @* */
        if (xml->attributes) {
            for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprAssert(qname->name[0] == '@');
                if (qname->name[1] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    mprRemoveItemAtPos(xml->attributes, next - 1);
                    item->parent = 0;
                    removed = 1;
                    next -= 1;
                }
            }
        }

    } else {
        /* name and * */
        if (xml->elements) {
            for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
                mprAssert(item->qname.name);
                if (qname->name[0] == '*' || strcmp(item->qname.name, qname->name) == 0) {
                    mprRemoveItemAtPos(xml->elements, next - 1);
                    item->parent = 0;
                    removed = 1;
                    next -= 1;
                }
            }
        }
    }
    return (removed) ? 0 : EJS_ERR;
}


static EjsVar *getXmlNodeName(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, xml->qname.name);
}



/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsXML  *xml;

    xml = (EjsXML*) ip->target;
    if (!ejsIsXML(xml)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < mprGetListCount(xml->elements); ip->index++) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getXmlIterator(Ejs *ejs, EjsVar *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, xml, (EjsNativeFunction) nextXmlKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsXML      *xml, *vp;

    xml = (EjsXML*) ip->target;
    if (!ejsIsXML(xml)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < mprGetListCount(xml->elements); ip->index++) {
        vp = (EjsXML*) mprGetItem(xml->elements, ip->index);
        if (vp == 0) {
            continue;
        }
        ip->index++;
        return (EjsVar*) vp;
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getXmlValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextXmlValue, 0, NULL);
}


static int getXmlPropertyCount(Ejs *ejs, EjsXML *xml)
{
    return mprGetListCount(xml->elements);
}


/*
 *  Lookup a property by name. There are 7 kinds of lookups:
 *       prop, @att, [prop], *, @*, .name, .@name
 */
static EjsVar *getXmlPropertyByName(Ejs *ejs, EjsXML *xml, EjsName *qname)
{
    EjsXML      *item, *result, *list;
    int         next, nextList;

    result = 0;

    mprAssert(xml->kind < 5);
#if UNUSED
    if (ejsIsCall(*ejs->frame->pc)) {
        return 0;
    }
#endif
#if WAS
    if (ejsIsCall(ejs->opcode)) {
        return 0;
    }
#endif

    if (isdigit((int) qname->name[0]) && allDigitsForXml(qname->name)) {
        /*
         *  Consider xml as a list with only one entry == xml. Then return the 0'th entry
         */
        return (EjsVar*) xml;
    }

    if (qname->name[0] == '@') {
        /* @ and @* */
        result = ejsCreateXMLList(ejs, xml, qname);
        if (xml->attributes) {
            for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprAssert(qname->name[0] == '@');
                if (qname->name[1] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    result = ejsAppendToXML(ejs, result, item);
                }
            }
        }

    } else if (qname->name[0] == '.') {
        /* Decenders (do ..@ also) */
        result = ejsXMLDescendants(ejs, xml, qname);

    } else {
        /* name and * */
        result = ejsCreateXMLList(ejs, xml, qname);
        if (xml->elements) {
            for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
        mprAssert(xml->kind !=24);
                if (item->kind == EJS_XML_LIST) {
                    list = item;
                    for (nextList = 0; (item = mprGetNextItem(list->elements, &nextList)) != 0; ) {
                        mprAssert(item->qname.name);
                        if (qname->name[0] == '*' || strcmp(item->qname.name, qname->name) == 0) {
                            result = ejsAppendToXML(ejs, result, item);
                        }
                    }

                } else {
                    mprAssert(item->qname.name);
                    if (qname->name[0] == '*' || strcmp(item->qname.name, qname->name) == 0) {
                        result = ejsAppendToXML(ejs, result, item);
                    }
                }
            }
        }
    }

    return (EjsVar*) result;
}


static EjsVar *invokeXmlOperator(Ejs *ejs, EjsXML *lhs, int opcode,  EjsXML *rhs)
{
    EjsVar      *result;

    if ((result = ejsCoerceOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
        return result;
    }

    switch (opcode) {
    case EJS_OP_COMPARE_EQ:
        return (EjsVar*) ejsCreateBoolean(ejs, deepCompare(lhs, rhs));

    case EJS_OP_COMPARE_NE:
        return (EjsVar*) ejsCreateBoolean(ejs, !deepCompare(lhs, rhs));

    default:
        return ejsObjectOperator(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs);
    }
}


/*
 *  Set a property attribute by name.
 */
static int setXmlPropertyAttributeByName(Ejs *ejs, EjsXML *xml, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *attribute, *rp, *xvalue, *lastElt;
    EjsString   *sv;
    EjsName     qn;
    char        *str;
    int         index, last, next, len;

    /*
     *  Attribute. If the value is an XML list, convert to a space separated string
     */
    xvalue = (EjsXML*) value;
    if (ejsIsXML(xvalue) && xvalue->kind == EJS_XML_LIST) {
        str = 0;
        len = 0;
        for (next = 0; (elt = mprGetNextItem(xvalue->elements, &next)) != 0; ) {
            sv = (EjsString*) ejsCastVar(ejs, (EjsVar*) elt, ejs->stringType);
            len = mprReallocStrcat(ejs, &str, -1, len, " ", sv->value, 0);
        }
        value = (EjsVar*) ejsCreateString(ejs, str);
        mprFree(str);

    } else {
        value = ejsCastVar(ejs, value, ejs->stringType);
    }
    mprAssert(ejsIsString(value));

    /*
     *  Find the first attribute that matches. Delete all other attributes of the same name.
     */
    index = 0;
    if (xml->attributes) {
        lastElt = 0;
        for (last = -1, index = -1; (elt = mprGetPrevItem(xml->attributes, &index)) != 0; ) {
            mprAssert(qname->name[0] == '@');
            if (strcmp(elt->qname.name, &qname->name[1]) == 0) {
                if (last >= 0) {
                    rp = mprGetItem(xml->attributes, last);
                    mprRemoveItemAtPos(xml->attributes, last);
                }
                last = index;
                lastElt = elt;
            }
        }
        if (lastElt) {
            /*
             *  Found a match. So replace its value
             */
            mprFree(lastElt->value);
            lastElt->value = mprStrdup(lastElt, ((EjsString*) value)->value);
            return last;

        } else {
            index = mprGetListCount(xml->attributes);
        }
    }
    //  TODO - namespace work to do here

    /*
     *  Not found. Create a new attribute node
     */
    mprAssert(ejsIsString(value));
    ejsName(&qn, 0, &qname->name[1]);
    attribute = ejsCreateXML(ejs, EJS_XML_ATTRIBUTE, &qn, xml, ((EjsString*) value)->value);
    if (xml->attributes == 0) {
        xml->attributes = mprCreateList(xml);
    }
    mprSetItem(xml->attributes, index, attribute);

    return index;
}


/*
 *  Create a value node. If the value is an XML node already, we are done. Otherwise, cast the value to a string
 *  and create a text child node containing the string value.
 */
static EjsXML *createValueNode(Ejs *ejs, EjsXML *elt, EjsVar *value)
{
    EjsXML      *text;
    EjsString   *str;

    if (ejsIsXML(value)) {
        return (EjsXML*) value;
    }

    str = (EjsString*) ejsCastVar(ejs, value, ejs->stringType);
    if (str == 0) {
        return 0;
    }

    if (mprGetListCount(elt->elements) == 1) {
        /*
         *  Update an existing text element
         */
        text = mprGetFirstItem(elt->elements);
        if (text->kind == EJS_XML_TEXT) {
            mprFree(text->value);
            text->value = mprStrdup(elt, str->value);
            return elt;
        }
    }

    /*
     *  Create a new text element
     */
    if (str->value && str->value[0] != '\0') {
        text = ejsCreateXML(ejs, EJS_XML_TEXT, NULL, elt, str->value);
        elt = ejsAppendToXML(ejs, elt, text);
    }
    return elt;
}


void ejsMarkXML(Ejs *ejs, EjsVar *parent, EjsXML *xml)
{
    EjsVar          *item;
    int             next;

    if (xml->parent && !xml->parent->var.visited) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) xml->parent);
    }
    if (xml->targetObject && !xml->targetObject->var.visited) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) xml->targetObject);
    }

    for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) item);
    }
    for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) item);
    }
}


/*
 *  Set a property by name. Implements the ECMA-357 [[put]] method.
 *  There are 7 kinds of qname's: prop, @att, [prop], *, @*, .name, .@name
 */
static int setXmlPropertyByName(Ejs *ejs, EjsXML *xml, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *xvalue, *rp, *lastElt;
    EjsVar      *originalValue;
    int         index, last;

    mprLog(ejs, 9, "XMLSet %s.%s = \"%s\"", xml->qname.name, qname->name,
        ((EjsString*) ejsCastVar(ejs, value, ejs->stringType))->value);

    if (isdigit((int) qname->name[0]) && allDigitsForXml(qname->name)) {
        ejsThrowTypeError(ejs, "Integer indicies for set are not allowed");
        return EJS_ERR;
    }

    if (xml->kind != EJS_XML_ELEMENT) {
        //  TODO spec requires this -- but why? -- surely throw?
        return 0;
    }

    /*
     *  Massage the value type.
     */
    originalValue = value;

    xvalue = (EjsXML*) value;
    if (ejsIsXML(xvalue)) {
        if (xvalue->kind == EJS_XML_LIST) {
            value = (EjsVar*) ejsDeepCopyXML(ejs, xvalue);

        } else if (xvalue->kind == EJS_XML_TEXT || xvalue->kind == EJS_XML_ATTRIBUTE) {
            value = ejsCastVar(ejs, originalValue, ejs->stringType);

        } else {
            value = (EjsVar*) ejsDeepCopyXML(ejs, xvalue);
        }

    } else {
        value = ejsCastVar(ejs, value, ejs->stringType);
    }

    if (qname->name[0] == '@') {
        return setXmlPropertyAttributeByName(ejs, xml, qname, value);
    }

    /*
     *  Delete redundant elements by the same name.
     */
    lastElt = 0;
    if (xml->elements) {
        for (last = -1, index = -1; (elt = mprGetPrevItem(xml->elements, &index)) != 0; ) {
            if (qname->name[0] == '*' || (elt->kind == EJS_XML_ELEMENT && strcmp(elt->qname.name, qname->name) == 0)) {
                /*
                 *  Must remove all redundant elements of the same name except the first one
                 */
                if (last >= 0) {
                    rp = mprGetItem(xml->elements, last);
                    rp->parent = 0;
                    mprRemoveItemAtPos(xml->elements, last);
                }
                last = index;
                lastElt = elt;
            }
        }
    }

    if (xml->elements == 0) {
        //  TODO - need routine to do this centrally so we can control the default number of elements in the list?
        xml->elements = mprCreateList(xml);
    }

    elt = lastElt;

    if (qname->name[0] == '*') {
        /*
         *  Special case when called from XMLList to update the value of an element
         */
        xml = createValueNode(ejs, xml, value);

    } else if (elt == 0) {
        /*
         *  Not found. New node required.
         */
        elt = ejsCreateXML(ejs, EJS_XML_ELEMENT, qname, xml, NULL);
        if (elt == 0) {
            return 0;
        }
        index = mprGetListCount(xml->elements);
        xml = ejsAppendToXML(ejs, xml, createValueNode(ejs, elt, value));

    } else {
        /*
         *  Update existing element.
         */
        xml = ejsSetXML(ejs, xml, index, createValueNode(ejs, elt, value));
    }

    if (xml == 0) {
        return EJS_ERR;
    }
    return index;
}


/*
 *  Deep compare
 */
static bool deepCompare(EjsXML *lhs, EjsXML *rhs)
{
    EjsXML      *l, *r;
    int         i;

    if (lhs == rhs) {
        return 1;
    }

    //  TODO - must compare all the namespaces?
    if (lhs->kind != rhs->kind) {
        return 0;

    } else  if (mprStrcmp(lhs->qname.name, rhs->qname.name) != 0) {
        return 0;

    } else if (mprGetListCount(lhs->attributes) != mprGetListCount(rhs->attributes)) {
        //  TODO - must compare all the attributes
        return 0;

    } else if (mprGetListCount(lhs->elements) != mprGetListCount(rhs->elements)) {
        //  TODO - must compare all the children
        return 0;

    } else if (mprStrcmp(lhs->value, rhs->value) != 0) {
        return 0;

    } else {
        for (i = 0; i < mprGetListCount(lhs->elements); i++) {
            l = mprGetItem(lhs->elements, i);
            r = mprGetItem(rhs->elements, i);
            if (! deepCompare(l, r)) {
                return 0;
            }
        }
    }
    return 1;
}


//  TODO - rename ejsGetXMLDescendants. Check all other names.
EjsXML *ejsXMLDescendants(Ejs *ejs, EjsXML *xml, EjsName *qname)
{
    EjsXML          *item, *result;
    int             next;

    result = ejsCreateXMLList(ejs, xml, qname);
    if (result == 0) {
        return 0;
    }

    if (qname->name[0] == '@') {
        if (xml->attributes) {
            for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprAssert(qname->name[0] == '@');
                if (qname->name[1] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    result = ejsAppendToXML(ejs, result, item);
                }
            }
        }

    } else {
        if (xml->elements) {
            for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
                if (qname->name[0] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    result = ejsAppendToXML(ejs, result, item);
                } else {
                    result = ejsAppendToXML(ejs, result, ejsXMLDescendants(ejs, item, qname));
                }
            }
        }
    }
    return result;
}


EjsXML *ejsDeepCopyXML(Ejs *ejs, EjsXML *xml)
{
    EjsXML      *root, *elt;
    int         next;

    if (xml == 0) {
        return 0;
    }

    if (xml->kind == EJS_XML_LIST) {
        root = ejsCreateXMLList(ejs, xml->targetObject, &xml->targetProperty);
    } else {
        root = ejsCreateXML(ejs, xml->kind, &xml->qname, NULL, xml->value);
    }
    if (root == 0) {
        return 0;
    }

    //  TODO - must copy inScopeNamespaces?

    if (xml->attributes) {
        root->attributes = mprCreateList(xml);
        for (next = 0; (elt = (EjsXML*) mprGetNextItem(xml->attributes, &next)) != 0; ) {
            elt = ejsDeepCopyXML(ejs, elt);
            if (elt) {
                elt->parent = root;
                mprAddItem(root->attributes, elt);
            }
        }
    }

    if (xml->elements) {
        root->elements = mprCreateList(root);
        for (next = 0; (elt = mprGetNextItem(xml->elements, &next)) != 0; ) {
            mprAssert(ejsIsXML(elt));
            elt = ejsDeepCopyXML(ejs, elt);
            if (elt) {
                elt->parent = root;
                mprAddItem(root->elements, elt);
            }
        }
    }

    if (mprHasAllocError(ejs)) {
        mprFree(root);
        return 0;
    }

    return root;
}

/*
 *  native function XML(value: Object = null)
 */
static EjsVar *xmlConstructor(Ejs *ejs, EjsXML *thisObj, int argc, EjsVar **argv)
{
    EjsVar      *arg;
    cchar       *str;

    //  TODO - should be also able to handle a File object

    if (thisObj == 0) {
        /*
         *  Called as a function - cast the arg
         */
        if (argc > 0){
            arg = ejsCastVar(ejs, argv[0], ejs->stringType);
            if (arg == 0) {
                return 0;
            }
        }
        thisObj = ejsCreateXML(ejs, 0, NULL, NULL, NULL);
    }

    if (argc == 0) {
        return (EjsVar*) thisObj;
    }

    arg = argv[0];
    mprAssert(arg);

    if (ejsIsNull(arg) || ejsIsUndefined(arg)) {
        return (EjsVar*) thisObj;
    }

    if (ejsIsObject(arg)) {
        arg = ejsCastVar(ejs, argv[0], ejs->stringType);
    }
    if (arg && ejsIsString(arg)) {
        str = ((EjsString*) arg)->value;
        if (str == 0) {
            return 0;
        }
        if (*str == '<') {
            /* XML Literal */
            ejsLoadXMLString(ejs, thisObj, str);

        } else {
            /* Load from file */
            loadXml(ejs, thisObj, argc, argv);
        }

    } else {
        ejsThrowArgError(ejs, "Bad type passed to XML constructor");
        return 0;
    }
    return (EjsVar*) thisObj;
}


static EjsVar *loadXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    MprFile     *file;
    MprXml      *xp;
    char        *filename;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    filename = ejsGetString(argv[0]);

    file = mprOpen(ejs, filename, O_RDONLY, 0664);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open: %s", filename);
        return 0;
    }

    //  TODO - convert to open/close
    xp = ejsCreateXmlParser(ejs, xml, filename);
    if (xp == 0) {
        ejsThrowMemoryError(ejs);
        mprFree(xp);
        mprFree(file);
        return 0;
    }
    mprXmlSetInputStream(xp, readFileData, (void*) file);

    if (mprXmlParse(xp) < 0) {
        if (! ejsHasException(ejs)) {
            ejsThrowIOError(ejs, "Can't parse XML file: %s\nDetails %s",  filename, mprXmlGetErrorMsg(xp));
        }
        mprFree(xp);
        mprFree(file);
        return 0;
    }

    mprFree(xp);
    mprFree(file);

    return 0;
}


static EjsVar *saveXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    MprBuf      *buf;
    MprFile     *file;
    cchar       *filename;
    int         bytes, len;

    if (argc != 1 || !ejsIsString(argv[0])) {
        ejsThrowArgError(ejs, "Bad args. Usage: save(filename);");
        return 0;
    }
    filename = ((EjsString*) argv[0])->value;

    /*
     *  Create a buffer to hold the output. All in memory.
     */
    buf = mprCreateBuf(ejs, MPR_BUFSIZE, -1);
    mprPutStringToBuf(buf, "<?xml version=\"1.0\"?>\n");

    /*
     * Convert XML to a string
     */
    if (ejsXMLToString(ejs, buf, xml, 0) < 0) {
        mprFree(buf);
        return 0;
    }

    file = mprOpen(ejs, filename,  O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open: %s, %d", filename,  mprGetOsError(ejs));
        return 0;
    }

    len = mprGetBufLength(buf);
    bytes = mprWrite(file, buf->start, len);
    if (bytes != len) {
        ejsThrowIOError(ejs, "Can't write to: %s", filename);
        mprFree(file);
        return 0;
    }
    mprWrite(file, "\n", 1);
    mprFree(buf);

    mprFree(file);

    return 0;
}


/*
 *  Convert the XML object to a string.
 *
 *  intrinsic function toString() : String
 */
static EjsVar *xmlToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (vp->type->helpers->castVar)(ejs, vp, ejs->stringType);
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic public override function get length(): int
 */
static EjsVar *xmlLength(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetListCount(xml->elements));
}


#if UNUSED
/*
 *  Set the length. TODO - what does this do?
 *  intrinsic public override function set length(value: int): void
 */
static EjsVar *setLength(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    int         length;

    mprAssert(ejsIsXML(xml));

    if (argc != 1) {
        ejsThrowArgError(ejs, "usage: obj.length = value");
        return 0;
    }
    length = ejsVarToInteger(ejs, argv[0]);

    if (length < ap->length) {
        for (i = length; i < ap->length; i++) {
            if (ejsSetProperty(ejs, (EjsVar*) ap, i, (EjsVar*) ejs->undefinedValue) < 0) {
                //  TODO - DIAG
                return 0;
            }
        }

    } else if (length > ap->length) {
        if (ejsSetProperty(ejs, (EjsVar*) ap, length - 1,  (EjsVar*) ejs->undefinedValue) < 0) {
            //  TODO - DIAG
            return 0;
        }
    }

    ap->length = length;
    return 0;
}
#endif

/*
 *  Set an indexed element to an XML value
 */
EjsXML *ejsSetXML(Ejs *ejs, EjsXML *xml, int index, EjsXML *node)
{
    EjsXML      *old;

    if (xml == 0 || node == 0) {
        return 0;
    }

    if (xml->elements == 0) {
        xml->elements = mprCreateList(xml);

    } else {
        old = (EjsXML*) mprGetItem(xml->elements, index);
        if (old && old != node) {
            old->parent = 0;
        }
    }

    if (xml->kind != EJS_XML_LIST) {
        node->parent = xml;
    }

    if (mprSetItem(xml->elements, index, node) < 0) {
        return 0;
    }

    return xml;
}


#if UNUSED
int ejsCopyName(MprCtx ctx, EjsName *to, EjsName *from)
{
/*
 *
 *  TODO -
 *
    mprFree((char*) to->name);
    mprFree((char*) to->space);

    to->name = mprStrdup(ctx, from->name);
    to->space = mprStrdup(ctx, from->space);
    if (to->name == 0 || to->space == 0) {
        return EJS_ERR;
    }
*/
    *to = *from;

    return 0;
}
#endif


EjsXML *ejsAppendToXML(Ejs *ejs, EjsXML *xml, EjsXML *node)
{
    EjsXML      *elt;
    int         next;

    if (xml == 0 || node == 0) {
        return 0;
    }
    if (xml->elements == 0) {
        xml->elements = mprCreateList(xml);
    }

    if (node->kind == EJS_XML_LIST) {
        for (next = 0; (elt = mprGetNextItem(node->elements, &next)) != 0; ) {
            if (xml->kind != EJS_XML_LIST) {
                elt->parent = xml;
            }
            mprAddItem(xml->elements, elt);
        }
        xml->targetObject = node->targetObject;
        xml->targetProperty = node->targetProperty;

    } else {
        if (xml->kind != EJS_XML_LIST) {
            node->parent = xml;
        }
        mprAddItem(xml->elements, node);
    }

    return xml;
}


int ejsAppendAttributeToXML(Ejs *ejs, EjsXML *parent, EjsXML *node)
{
    if (parent->attributes == 0) {
        parent->attributes = mprCreateList(parent);
    }
    node->parent = parent;
    return mprAddItem(parent->attributes, node);
}


static int readFileData(MprXml *xp, void *data, char *buf, int size)
{
    mprAssert(xp);
    mprAssert(data);
    mprAssert(buf);
    mprAssert(size > 0);

    return mprRead((MprFile*) data, buf, size);
}


static int readStringData(MprXml *xp, void *data, char *buf, int size)
{
    EjsXmlState *parser;
    int         rc, len;

    mprAssert(xp);
    mprAssert(buf);
    mprAssert(size > 0);

    parser = (EjsXmlState*) xp->parseArg;

    if (parser->inputPos < parser->inputSize) {
        len = min(size, (parser->inputSize - parser->inputPos));
        rc = mprMemcpy(buf, size, &parser->inputBuf[parser->inputPos], len);
        parser->inputPos += len;
        return rc;
    }
    return 0;
}


static bool allDigitsForXml(cchar *name)
{
    cchar   *cp;

    for (cp = name; *cp; cp++) {
        if (!isdigit((int) *cp) || *cp == '.') {
            return 0;
        }
    }
    return 1;
}


EjsXML *ejsCreateXML(Ejs *ejs, int kind, EjsName *qname, EjsXML *parent, cchar *value)
{
    EjsXML      *xml;

    xml = (EjsXML*) ejsAllocVar(ejs, ejs->xmlType, 0);
    if (xml == 0) {
        return 0;
    }

    if (qname) {
        xml->qname.name = mprStrdup(xml, qname->name);
        xml->qname.space = mprStrdup(xml, qname->space);
    }

    xml->kind = kind;
    xml->parent = parent;
    if (value) {
        xml->value = mprStrdup(xml, value);
    }
    //  TODO - should we create the elements list here?
    return xml;
}


EjsXML *ejsConfigureXML(Ejs *ejs, EjsXML *xml, int kind, cchar *name, EjsXML *parent, cchar *value)
{
    mprFree((char*) xml->qname.name);
    //  TODO - RC
    xml->qname.name = mprStrdup(xml, name);
    xml->kind = kind;
    xml->parent = parent;
    if (value) {
        mprFree(xml->value);
        //  TODO - RC
        xml->value = mprStrdup(xml, value);
    }
    return xml;
}


/*
 *  Support routine. Not an class method
 */
void ejsLoadXMLString(Ejs *ejs, EjsXML *xml, cchar *xmlString)
{
    EjsXmlState *parser;
    MprXml      *xp;

    xp = ejsCreateXmlParser(ejs, xml, "string");
    parser = mprXmlGetParseArg(xp);

    parser->inputBuf = xmlString;
    parser->inputSize = (int) strlen(xmlString);

    mprXmlSetInputStream(xp, readStringData, (void*) 0);

    if (mprXmlParse(xp) < 0 && !ejsHasException(ejs)) {
        ejsThrowSyntaxError(ejs, "Can't parse XML string: %s",  mprXmlGetErrorMsg(xp));
    }

    mprFree(xp);
}


int ejsCreateXMLType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "XML"), ejs->objectType, sizeof(EjsXML), ES_XML,
        ES_XML_NUM_CLASS_PROP, ES_XML_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    if (type == 0) {
        return EJS_ERR;
    }
    ejs->xmlType = type;
    type->nobind = 1;

    /*
     *  Define the helper functions.
     */
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneXml;
    type->helpers->castVar = (EjsCastVarHelper) castXml;
    type->helpers->createVar = (EjsCreateVarHelper) createXml;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyXml;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getXmlPropertyByName;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getXmlPropertyCount;
    type->helpers->deletePropertyByName = (EjsDeletePropertyByNameHelper) deleteXmlPropertyByName;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeXmlOperator;
    type->helpers->markVar = (EjsMarkVarHelper) ejsMarkXML;
    type->helpers->setPropertyByName = (EjsSetPropertyByNameHelper) setXmlPropertyByName;
    
    return 0;
}


int ejsConfigureXMLType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->xmlType;

    /*
     *  Define the XML class methods
     */
    ejsBindMethod(ejs, type, ES_XML_XML, (EjsNativeFunction) xmlConstructor);
    ejsBindMethod(ejs, type, ES_XML_load, (EjsNativeFunction) loadXml);
    ejsBindMethod(ejs, type, ES_XML_save, (EjsNativeFunction) saveXml);
    ejsBindMethod(ejs, type, ES_XML_name, (EjsNativeFunction) getXmlNodeName);

    /*
     *  Override these methods
     */
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) xmlLength);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) xmlToString);

    ejsBindMethod(ejs, type, ES_Object_get, getXmlIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getXmlValues);

#if FUTURE
    ejsBindMethod(ejs, type, ES_XML_parent, parent);
    ejsBindMethod(ejs, type, "valueOf", valueOf, NULL);
#endif
    return 0;
}


#else
void __ejsXMLDummy() {}
#endif /* BLD_FEATURE_EJS_E4X */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/xml/ejsXML.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/xml/ejsXMLList.c"
 */
/************************************************************************/

/**
 *  ejsXMLList.c - E4X XMLList type.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_E4X

/*
 *  XMLList methods
 */

#if UNUSED
static EjsVar   *valueOf(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *xlLength(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *toXmlString(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *appendChild(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *attributes(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *child(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *elements(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *comments(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *decendants(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *elements(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *insertChildAfter(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *insertChildBefore(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *replace(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *setName(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *text(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);

#endif

static bool allDigitsForXmlList(cchar *name);
static EjsXML *resolve(Ejs *ejs, EjsXML *obj);
static EjsXML *shallowCopy(Ejs *ejs, EjsXML *xml);


static EjsXML *createXmlListVar(Ejs *ejs, EjsType *type, int size)
{
    return (EjsXML*) ejsCreateXMLList(ejs, NULL, NULL);
}


static void destroyXmlList(Ejs *ejs, EjsXML *list)
{
    ejsFreeVar(ejs, (EjsVar*) list);
}


static EjsVar *cloneXmlList(Ejs *ejs, EjsXML *list, bool deep)
{
    EjsXML  *newList;

    //  TODO - implement deep copy
    newList = (EjsXML*) ejsCreateVar(ejs, list->var.type, 0);
    if (newList == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }
    //  TODO incomplete
    return (EjsVar*) newList;
}


/*
 *  Cast the object operand to a primitive type
 */
static EjsVar *xlCast(Ejs *ejs, EjsXML *vp, EjsType *type)
{
    MprBuf      *buf;
    EjsVar      *result;
    EjsXML      *elt, *item;
    int         next;

    switch (type->id) {
    case ES_Object:
    case ES_XML:
        return (EjsVar*) vp;

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_Number:
        result = xlCast(ejs, vp, ejs->stringType);
        result = (EjsVar*) ejsToNumber(ejs, result);
        return result;

    case ES_String:
        buf = mprCreateBuf(ejs, MPR_BUFSIZE, -1);
        if (mprGetListCount(vp->elements) == 1) {
            elt = mprGetFirstItem(vp->elements);
            if (elt->kind == EJS_XML_ELEMENT) {
                if (elt->elements == 0) {
                    return (EjsVar*) ejs->emptyStringValue;
                }
                if (elt->elements && mprGetListCount(elt->elements) == 1) {
                    //  TODO - what about PI and comments?
                    item = mprGetFirstItem(elt->elements);
                    if (item->kind == EJS_XML_TEXT) {
                        return (EjsVar*) ejsCreateString(ejs, item->value);
                    }
                }
            }
        }
        for (next = 0; (elt = mprGetNextItem(vp->elements, &next)) != 0; ) {
            if (ejsXMLToString(ejs, buf, elt, -1) < 0) {
                mprFree(buf);
                return 0;
            }
        }
        result = (EjsVar*) ejsCreateString(ejs, (char*) buf->start);
        mprFree(buf);
        return result;

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


//  TODO - seems the return code for delete should be boolean?

static int deleteXmlListPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname)
{
    EjsXML      *elt;
    int         index, next;

    if (isdigit((int) qname->name[0]) && allDigitsForXmlList(qname->name)) {
        index = atoi(qname->name);

        elt = (EjsXML*) mprGetItem(list->elements, index);
        if (elt) {
            if (elt->parent) {
                if (elt->kind == EJS_XML_ATTRIBUTE) {
                    ejsDeletePropertyByName(ejs, (EjsVar*) elt->parent, &elt->qname);
                } else {
                    //  TODO - let q be the property of parent where parent[q] == x[i]
                    mprRemoveItem(elt->parent->elements, elt);
                    elt->parent = 0;
                }
            }
        }
        //  Spec says return true even if index is out of range. We return 0 for true and < 0 for false.
        //  TODO - should ejs throw?
        return 0;
    }

    for (next = 0; (elt = mprGetNextItem(list->elements, &next)) != 0; ) {
        if (elt->kind == EJS_XML_ELEMENT && elt->parent) {
            ejsDeletePropertyByName(ejs, (EjsVar*) elt->parent, qname);
        }
    }

    return 0;
}


static int getXmlListPropertyCount(Ejs *ejs, EjsXML *list)
{
    return mprGetListCount(list->elements);
}


/*
 *  Lookup a property by name. There are 7 kinds of lookups:
 *       prop, @att, [prop], *, @*, .name, .@name
 */
static EjsVar *getXmlListPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname)
{
    EjsXML      *result, *subList, *item;
    int         nextItem;

#if UNUSED
    if (ejsIsCall(ejs->opcode)) {
        return 0;
    }
#endif

    /*
     *  Get the n'th item in the list
     */
    if (isdigit((int) qname->name[0]) && allDigitsForXmlList(qname->name)) {
        return mprGetItem(list->elements, atoi(qname->name));
    }

    result = ejsCreateXMLList(ejs, list, qname);

    /*
     *  Build a list of all the elements that themselves have a property qname.
     */
    for (nextItem = 0; (item = mprGetNextItem(list->elements, &nextItem)) != 0; ) {
        if (item->kind == EJS_XML_ELEMENT) {
            subList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) item, qname);
            mprAssert(ejsIsXML(subList));
            ejsAppendToXML(ejs, result, subList);

        } else {
            //  TODO - do we ever get a list in a list?
            mprAssert(0);
        }
    }

    return (EjsVar*) result;
}


static EjsVar *getXmlListNodeName(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, xml->qname.name);
}



/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlListKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsXML  *xml;

    xml = (EjsXML*) ip->target;
    if (!ejsIsXML(xml)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < mprGetListCount(xml->elements); ip->index++) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getXmlListIterator(Ejs *ejs, EjsVar *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, xml, (EjsNativeFunction) nextXmlListKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlListValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsXML      *xml, *vp;

    xml = (EjsXML*) ip->target;
    if (!ejsIsXML(xml)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < mprGetListCount(xml->elements); ip->index++) {
        vp = (EjsXML*) mprGetItem(xml->elements, ip->index);
        if (vp == 0) {
            continue;
        }
        ip->index++;
        return (EjsVar*) vp;
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getXmlListValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextXmlListValue, 0, NULL);
}


#if OLD
/*
 *  Handle all core operators. We currenly handle only === and !==
 *  TODO. Must implement: +, -, <, >, <=, >=, ==, ===, !=, !==, &, |
 */
static EjsVar *invokeOperator(Ejs *ejs, EjsXML *lhs, int opCode,  EjsXML *rhs)
{
    EjsVar      *l, *r;
    bool        boolResult;

    mprAssert(ejsIsXML(lhs));
    mprAssert(ejsIsXML(rhs));

    //  TODO - Complete
    switch (opCode) {
    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
        boolResult = (lhs == rhs);
        break;

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        boolResult = !(lhs == rhs);
        break;

    default:
        /*
         *  Cast to strings and re-invoke
         */
        l = ejsCastVar(ejs, (EjsVar*) lhs, ejs->stringType);
        r = ejsCastVar(ejs, (EjsVar*) rhs, ejs->stringType);
        return ejsInvokeOperator(ejs, l, opCode, r);
    }
    return (EjsVar*) ejsCreateBoolean(ejs, boolResult);
}
#endif


/*
 *  Set an alpha property by name.
 */
static int setAlphaPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *targetObject;
    int         count;

    targetObject = 0;

    count = ejsGetPropertyCount(ejs, (EjsVar*) list);
    if (count > 1) {
        //  TODO - why no error in spec?
        mprAssert(0);
        return 0;
    }

    if (count == 0) {
        /*
         *  Empty list so resolve the real target object and append it to the list.
         */
        targetObject = resolve(ejs, list);
        if (targetObject == 0) {
            return 0;
        }
        if (ejsGetPropertyCount(ejs, (EjsVar*) targetObject) != 1) {
            return 0;
        }
        ejsAppendToXML(ejs, list, targetObject);
    }

    /*
     *  Update the element
     */
    mprAssert(ejsGetPropertyCount(ejs, (EjsVar*) list) == 1);
    elt = mprGetItem(list->elements, 0);                        //  TODO OPT - GetFirstItem
    mprAssert(elt);
    ejsSetPropertyByName(ejs, (EjsVar*) elt, qname, value);

    return 0;
}


static EjsXML *createElement(Ejs *ejs, EjsXML *list, EjsXML *targetObject, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *last, *attList;
    int         index;
    int         j;

    if (targetObject && ejsIsXML(targetObject) && targetObject->kind == EJS_XML_LIST) {

        /*
         *  If the target is a list it must have 1 element. So switch to it.
         *  TODO - could we get resolve to do this?
         */
        if (mprGetListCount(targetObject->elements) != 1) {
            /* Spec says so - TODO why no error? */
            return 0;
        }
        targetObject = mprGetFirstItem(targetObject->elements);
    }

    /*
     *  Return if the target object is not an XML element
     */
    if (!ejsIsXML(targetObject) || targetObject->kind != EJS_XML_ELEMENT) {
            /* Spec says so - TODO why no error? */
        return 0;
    }

    elt = ejsCreateXML(ejs, EJS_XML_ELEMENT, &list->targetProperty, targetObject, NULL);

    if (list->targetProperty.name && list->targetProperty.name[0] == '@') {
        elt->kind = EJS_XML_ATTRIBUTE;
        attList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) targetObject, &list->targetProperty);
        if (attList && mprGetListCount(attList->elements) > 0) {
            /* Spec says so. But this surely means you can't update an attribute? */
            return 0;
        }
    } else if (list->targetProperty.name == 0 || qname->name[0] == '*') {
        elt->kind = EJS_XML_TEXT;
        elt->qname.name = 0;
    }

    index = mprGetListCount(list->elements);

    if (elt->kind != EJS_XML_ATTRIBUTE) {
        if (targetObject) {
            if (index > 0) {
                /*
                 *  Find the place of the last list item in the resolved target object.
                 */
                last = mprGetItem(list->elements, index - 1);
                j = mprLookupItem(targetObject->elements, last);

            } else {
                j = mprGetListCount(targetObject->elements) - 1;
            }
            //  TODO - really need to wrap this ejsInsertXML(EjsXML *xml, int index, EjsXML *node)
            if (targetObject->elements == 0) {
                targetObject->elements = mprCreateList(targetObject);
            }
            /*
             *  Insert into the target object
             */
            mprInsertItemAtPos(targetObject->elements, j + 1, elt);
        }

        if (ejsIsXML(value)) {
            if (((EjsXML*) value)->kind == EJS_XML_LIST) {
                elt->qname = ((EjsXML*) value)->targetProperty;
            } else {
                elt->qname = ((EjsXML*) value)->qname;
            }
        }

        /*
         *  Insert into the XML list
         */
        mprSetItem(list->elements, index, elt);
    }

    return (EjsXML*) mprGetItem(list->elements, index);
}


/*
 *  Update an existing element
 */
static int updateElement(Ejs *ejs, EjsXML *list, EjsXML *elt, int index, EjsVar *value)
{
    EjsXML      *node;
    EjsName     name;
    int         i, j;

    if (!ejsIsXML(value)) {
        /* Not XML or XMLList -- convert to string */
        value = ejsCastVar(ejs, value, ejs->stringType);                //  TODO - seem to be doing this in too many places
    }
    mprSetItem(list->elements, index, value);

    if (elt->kind == EJS_XML_ATTRIBUTE) {
        mprAssert(ejsIsString(value));
        i = mprLookupItem(elt->parent->elements, elt);
        ejsSetXML(ejs, elt->parent, i, elt);
        //  TODO - why do this. Doesn't above do this?
        ejsSetPropertyByName(ejs, (EjsVar*) elt->parent, &elt->qname, value);
        mprFree(elt->value);
        elt->value = mprStrdup(elt, ((EjsString*) value)->value);
    }

    if (ejsIsXML(value) && ((EjsXML*) value)->kind == EJS_XML_LIST) {
        value = (EjsVar*) shallowCopy(ejs, (EjsXML*) value);
        if (elt->parent) {
            index = mprLookupItem(elt->parent->elements, elt);
            for (j = 0; j < mprGetListCount(((EjsXML*) value)->elements); j++) {
                mprInsertItemAtPos(elt->parent->elements, index, value);
            }
        }

    } else if (ejsIsXML(value) || elt->kind != EJS_XML_ELEMENT) {
        if (elt->parent) {
            index = mprLookupItem(elt->parent->elements, elt);
            mprSetItem(elt->parent->elements, index, value);
            if (ejsIsString(value)) {
                node = ejsCreateXML(ejs, EJS_XML_TEXT, NULL, list, ((EjsString*) value)->value);
                mprSetItem(list->elements, index, node);

            } else {
                mprSetItem(list->elements, index, value);
            }
        }

    } else {
        ejsName(&name, 0, "*");
        ejsSetPropertyByName(ejs, (EjsVar*) elt, &name, value);
    }

    return index;
}


/*
 *  Set a property by name.
 */
static int setXmlListPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *targetObject;
    int         index;

    if (!isdigit((int) qname->name[0])) {
        return setAlphaPropertyByName(ejs, list, qname, value);
    }

    /*
     *  Numeric property
     */
    targetObject = 0;
    if (list->targetObject) {
        /*
         *  Find the real underlying target object. May be an XML object or XMLList if it contains multiple elements.
         */
        targetObject = resolve(ejs, list->targetObject);
        if (targetObject == 0) {
            /* Spec says so - TODO why no error? */
            return 0;
        }
    }

    index = atoi(qname->name);
    if (index >= mprGetListCount(list->elements)) {
        /*
         *  Create, then fall through to update
         */
        elt = createElement(ejs, list, targetObject, qname, value);
        if (elt == 0) {
            return 0;
        }

    } else {
        elt = mprGetItem(list->elements, index);
    }
    mprAssert(elt);

    updateElement(ejs, list, elt, index, value);

    return index;
}




static bool allDigitsForXmlList(cchar *name)
{
    cchar   *cp;

    for (cp = name; *cp; cp++) {
        if (!isdigit((int) *cp) || *cp == '.') {
            return 0;
        }
    }
    return 1;
}


static EjsXML *shallowCopy(Ejs *ejs, EjsXML *xml)
{
    EjsXML      *root, *elt;
    int         next;

    mprAssert(xml->kind == EJS_XML_LIST);

    if (xml == 0) {
        return 0;
    }

    root = ejsCreateXMLList(ejs, xml->targetObject, &xml->targetProperty);
    if (root == 0) {
        return 0;
    }

    if (xml->elements) {
        root->elements = mprCreateList(root);
        for (next = 0; (elt = mprGetNextItem(xml->elements, &next)) != 0; ) {
            mprAssert(ejsIsXML(elt));
            if (elt) {
                mprAddItem(root->elements, elt);
            }
        }
    }

    if (mprHasAllocError(ejs)) {
        mprFree(root);
        return 0;
    }

    return root;
}


/*
 *  Resolve empty XML list objects to an actual XML object. This is used by SetPropertyByName to find the actual object to update.
 *  This method resolves the value of empty XMLLists. If the XMLList is not empty, the list will be returned. If list is empty,
 *  this method attempts to create an element based on the list targetObject and targetProperty.
 */
static EjsXML *resolve(Ejs *ejs, EjsXML *xml)
{
    EjsXML  *targetObject, *targetPropertyList;

    if (!ejsIsXML(xml) || xml->kind != EJS_XML_LIST) {
        /* Resolved to an XML object */
        return xml;
    }

    if (mprGetListCount(xml->elements) > 0) {
        /* Resolved to a list of items */
        return xml;
    }

    if (xml->targetObject == 0 || xml->targetProperty.name == 0 || xml->targetProperty.name[0] == '*') {
        /* End of chain an no more target objects */
        return 0;
    }

    targetObject = resolve(ejs, xml->targetObject);
    if (targetObject == 0) {
        return 0;
    }
    //  TODO - OPT. targetPropertyList is also being created below.
    targetPropertyList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) targetObject, &xml->targetProperty);
    if (targetPropertyList == 0) {
        return 0;
    }

    if (ejsGetPropertyCount(ejs, (EjsVar*) targetPropertyList) == 0) {
        /*
         *  Property does not exist in the target.
         */
        if (targetObject->kind == EJS_XML_LIST && ejsGetPropertyCount(ejs, (EjsVar*) targetObject) > 1) {
            return 0;
        }
        /*
         *  Create the property as an element (The text value will be optimized away).
         *  TODO - OPT. Need an empty string value in EjsFiber.
         */
        ejsSetPropertyByName(ejs, (EjsVar*) targetObject, &xml->targetProperty, (EjsVar*) ejsCreateString(ejs, ""));
        targetPropertyList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) targetObject, &xml->targetProperty);
    }
    return targetPropertyList;
}



static EjsVar *xmlListConstructor(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
#if FUTURE
    EjsVar      *vp;
    cchar       *str;

    if (argc == 1) {
        vp = argv[0];

        if (ejsIsObject(vp)) {
            /* Convert DOM to XML. Not implemented */;

        } else if (ejsIsString(vp)) {
            str = ((EjsString*) vp)->value;
            if (str == 0) {
                return 0;
            }
            if (*str == '<') {
                /* XML Literal */
                return loadXmlString(ejs, (EjsXML*) thisObj, str);

            } else {
                /* Load from file */
                return load(ejs, (EjsXML*) thisObj, argc, argv);
            }
        } else {
            ejsThrowArgError(ejs, "Bad type passed to XML constructor");
            return 0;
        }
    }
#endif
    return (EjsVar*) thisObj;
}


/*
 *  Convert the XML object to a string.
 *
 *  intrinsic function toString() : String
 */
static EjsVar *xmlListToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (vp->type->helpers->castVar)(ejs, vp, ejs->stringType);
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic public override function get length(): int
 */

static EjsVar *xlLength(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetListCount(xml->elements));
}


#if FUTURE
/*
 *  Set the length. TODO - what does this do?
 *  intrinsic public override function set length(value: int): void
 */
static EjsVar *setLength(Ejs *ejs, EjsXMLList *xml, int argc, EjsVar **argv)
{
    int         length;

    mprAssert(ejsIsXMLList(xml));

    if (argc != 1) {
        ejsThrowArgError(ejs, "usage: obj.length = value");
        return 0;
    }
    length = ejsVarToInteger(ejs, argv[0]);

#if UNUSED
    if (length < ap->length) {
        for (i = length; i < ap->length; i++) {
            if (ejsSetProperty(ejs, (EjsVar*) ap, i, (EjsVar*) ejs->undefinedValue) < 0) {
                //  TODO - DIAG
                return 0;
            }
        }

    } else if (length > ap->length) {
        if (ejsSetProperty(ejs, (EjsVar*) ap, length - 1,  (EjsVar*) ejs->undefinedValue) < 0) {
            //  TODO - DIAG
            return 0;
        }
    }

    ap->length = length;
#endif

    return 0;
}
#endif



EjsXML *ejsCreateXMLList(Ejs *ejs, EjsXML *targetObject, EjsName *targetProperty)
{
    EjsType     *type;
    EjsXML      *list;

    type = ejs->xmlListType;

    list = (EjsXML*) ejsAllocVar(ejs, type, 0);
    if (list == 0) {
        return 0;
    }

    list->kind = EJS_XML_LIST;
    list->elements = mprCreateList(list);
    list->targetObject = targetObject;

    if (targetProperty) {
        list->targetProperty.name = mprStrdup(list, targetProperty->name);
    }

#if NOT_NEEDED
    /*
     *  Temporary until we have namespaces
     */
    char        *cp;
    for (cp = name; *cp; cp++) {
        if (*cp == ':') {
            *cp = '_';
        }
    }
#endif

    return list;
}


int ejsCreateXMLListType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "XMLList"), ejs->objectType, sizeof(EjsXML), 
        ES_XMLList, ES_XMLList_NUM_CLASS_PROP, ES_XMLList_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    if (type == 0) {
        return EJS_ERR;
    }
    ejs->xmlListType = type;

    type->nobind = 1;

    /*
     *  Define the helper functions.
     */
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneXmlList;
    type->helpers->castVar = (EjsCastVarHelper) xlCast;
    type->helpers->createVar = (EjsCreateVarHelper) createXmlListVar;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyXmlList;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getXmlListPropertyByName;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getXmlListPropertyCount;
    type->helpers->deletePropertyByName = (EjsDeletePropertyByNameHelper) deleteXmlListPropertyByName;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) ejsObjectOperator;
    type->helpers->markVar = (EjsMarkVarHelper) ejsMarkXML;
    type->helpers->setPropertyByName = (EjsSetPropertyByNameHelper) setXmlListPropertyByName;
    return 0;
}


int ejsConfigureXMLListType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->xmlListType;

    /*
     *  Define the XMLList class methods
     */
    ejsBindMethod(ejs, type, ES_XMLList_XMLList, (EjsNativeFunction) xmlListConstructor);
    ejsBindMethod(ejs, type, ES_XML_name, (EjsNativeFunction) getXmlListNodeName);

    /*
     *  Override these methods
     */
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) xmlListToString);
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) xlLength);

    ejsBindMethod(ejs, type, ES_Object_get, getXmlListIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getXmlListValues);
#if FUTURE
    ejsBindMethod(ejs, type, "name", name, NULL);
    ejsBindMethod(ejs, type, "valueOf", valueOf, NULL);
#endif
    return 0;
}


#else
void __ejsXMLListDummy() {}
#endif /* BLD_FEATURE_EJS_E4X */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/xml/ejsXMLList.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../types/xml/ejsXMLLoader.c"
 */
/************************************************************************/

/**
 *  ejsXMLLoader.c - Load and save XML data.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_E4X


static void indent(MprBuf *bp, int level);
static int  parserHandler(MprXml *xp, int state, cchar *tagName, cchar *attName, cchar *value);


MprXml *ejsCreateXmlParser(Ejs *ejs, EjsXML *xml, cchar *filename)
{
    EjsXmlState *parser;
    MprXml      *xp;
    
    xp = mprXmlOpen(ejs, MPR_BUFSIZE, EJS_E4X_BUF_MAX);
    mprAssert(xp);

    /*
     *  Create the parser stack
     */
    parser = mprAllocObjZeroed(xp, EjsXmlState);
    if (parser == 0) {
        mprFree(xp);
        return 0;
    }
    parser->ejs = ejs;
    parser->nodeStack[0].obj = xml;
    
    //  TODO - these 2 are not really needed. Can use ejs->
    parser->xmlType = ejs->xmlType;
    parser->xmlListType = ejs->xmlListType;
    parser->filename = filename;

    mprXmlSetParseArg(xp, parser);
    mprXmlSetParserHandler(xp, parserHandler);

    return xp;
}



/*
 *  XML parsing callback. Called for each elt and attribute/value pair. 
 *  For speed, we handcraft the object model here rather than calling 
 *  putXmlProperty.
 *
 *  "<!-- txt -->"      parserHandler(, , MPR_XML_COMMENT);
 *  "<elt"              parserHandler(, , MPR_XML_NEW_ELT);
 *  "...att=value"      parserHandler(, , MPR_XML_NEW_ATT);
 *  "<elt ...>"         parserHandler(, , MPR_XML_ELT_DEFINED);
 *  "<elt/>"            parserHandler(, , MPR_XML_SOLO_ELT_DEFINED);
 *  "<elt> ...<"        parserHandler(, , MPR_XML_ELT_DATA);
 *  "...</elt>"         parserHandler(, , MPR_XML_END_ELT);
 *
 *  Note: we recurse on every new nested elt.
 */

static int parserHandler(MprXml *xp, int state, cchar *tagName, cchar *attName, cchar *value)
{
    Ejs             *ejs;
    EjsXmlState     *parser;
    EjsXmlTagState  *tos;
    EjsName         qname;
    EjsXML          *xml, *node, *parent;

    parser = (EjsXmlState*) xp->parseArg;
    ejs = parser->ejs;
    tos = &parser->nodeStack[parser->topOfStack];
    xml = tos->obj;
    
    mprAssert(xml);

    mprAssert(state >= 0);
    mprAssert(tagName && *tagName);

    switch (state) {
    case MPR_XML_PI:
        node = ejsCreateXML(ejs, EJS_XML_PROCESSING, NULL, xml, value);
        ejsAppendToXML(ejs, xml, node);
        break;

    case MPR_XML_COMMENT:
        node = ejsCreateXML(ejs, EJS_XML_COMMENT, NULL, xml, value);
        ejsAppendToXML(ejs, xml, node);
        break;

    case MPR_XML_NEW_ELT:
        if (parser->topOfStack > E4X_MAX_NODE_DEPTH) {
            ejsThrowSyntaxError(ejs,  "XML nodes nested too deeply in %s at line %d", parser->filename, mprXmlGetLineNumber(xp));
            return MPR_ERR_BAD_SYNTAX;
        }
        if (xml->kind <= 0) {
            ejsConfigureXML(ejs, xml, EJS_XML_ELEMENT, tagName, xml, NULL);
        } else {
            ejsName(&qname, 0, tagName);
            xml = ejsCreateXML(ejs, EJS_XML_ELEMENT, &qname, xml, NULL);
            tos = &parser->nodeStack[++(parser->topOfStack)];
            tos->obj = (EjsXML*) xml;
            tos->attributes = 0;
            tos->comments = 0;
        }
        break;

    case MPR_XML_NEW_ATT:
        ejsName(&qname, 0, attName);
        node = ejsCreateXML(ejs, EJS_XML_ATTRIBUTE, &qname, xml, value);
        //  TODO - rc
        ejsAppendAttributeToXML(ejs, xml, node);
        //  TODO RC
        break;

    case MPR_XML_SOLO_ELT_DEFINED:
        if (parser->topOfStack > 0) {
            parent = parser->nodeStack[parser->topOfStack - 1].obj;
            //  TODO - rc
            ejsAppendToXML(ejs, parent, xml);
            parser->topOfStack--;
            mprAssert(parser->topOfStack >= 0);
            tos = &parser->nodeStack[parser->topOfStack];
        }
        break;

    case MPR_XML_ELT_DEFINED:
        if (parser->topOfStack > 0) {
            parent = parser->nodeStack[parser->topOfStack - 1].obj;
            //  TODO - rc
            ejsAppendToXML(ejs, parent, xml);
        }
        break;

    case MPR_XML_ELT_DATA:
    case MPR_XML_CDATA:
        ejsName(&qname, 0, attName);
        node = ejsCreateXML(ejs, EJS_XML_TEXT, &qname, xml, value);
        //  TODO - rc
        ejsAppendToXML(ejs, xml, node);
        break;

    case MPR_XML_END_ELT:
        /*
         *  This is the closing element in a pair "<x>...</x>".
         *  Pop the stack frame off the elt stack
         */
        if (parser->topOfStack > 0) {
            parser->topOfStack--;
            mprAssert(parser->topOfStack >= 0);
            tos = &parser->nodeStack[parser->topOfStack];
        }
        break;

    default:
        ejsThrowSyntaxError(ejs, "XML error in %s at %d\nDetails %s", parser->filename, mprXmlGetLineNumber(xp), mprXmlGetErrorMsg(xp));
        mprAssert(0);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}



#if UNUSED
static bool checkTagName(char *name)
{
    char    *cp;

    for (cp = name; *cp; cp++) {
        if (!isalnum(*cp) && *cp != '_' && *cp != '$' && *cp != '@') {
            return 0;
        }
    }
    return 1;
}
#endif



int ejsXMLToString(Ejs *ejs, MprBuf *buf, EjsXML *node, int indentLevel)
{
    EjsXML      *xml, *child, *attribute, *elt;
    int         sawElements, next;
    
    if (node->var.visited) {
        return 0;
    }
    node->var.visited = 1;

    if (node->kind == EJS_XML_LIST) {
        for (next = 0; (elt = mprGetNextItem(node->elements, &next)) != 0; ) {
            ejsXMLToString(ejs, buf, elt, indentLevel);
        }
        return 0;
    }
    
    mprAssert(ejsIsXML(node));
    xml = (EjsXML*) node;
    
    switch (xml->kind) {
    case EJS_XML_ELEMENT:
        /*
         *  XML object is complex (has elements) so return full XML content.
         */
        if (indentLevel > 0) {
            mprPutCharToBuf(buf, '\n');
        }
        indent(buf, indentLevel);

        mprPutFmtToBuf(buf, "<%s", xml->qname.name);
        if (xml->attributes) {
            for (next = 0; (attribute = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprPutFmtToBuf(buf, " %s=\"%s\"",  attribute->qname.name, attribute->value);
            }
        }
        
        sawElements = 0;
        if (xml->elements) {
            mprPutStringToBuf(buf, ">"); 
            for (next = 0; (child = mprGetNextItem(xml->elements, &next)) != 0; ) {
                if (child->kind != EJS_XML_TEXT) {
                    sawElements++;
                }
    
                /* Recurse */
                if (ejsXMLToString(ejs, buf, child, indentLevel < 0 ? -1 : indentLevel + 1) < 0) {
                    return -1;
                }
            }
            if (sawElements && indentLevel >= 0) {
                mprPutCharToBuf(buf, '\n');
                indent(buf, indentLevel);
            }
            mprPutFmtToBuf(buf, "</%s>", xml->qname.name);
            
        } else {
            /* Solo */
            mprPutStringToBuf(buf, "/>");
        }
        break;
        
    case EJS_XML_COMMENT:
        mprPutCharToBuf(buf, '\n');
        indent(buf, indentLevel);
        mprPutFmtToBuf(buf, "<!--%s -->", xml->value);
        break;
        
    case EJS_XML_ATTRIBUTE:
        /*
         *  Only here when converting solo attributes to a string
         */
        mprPutStringToBuf(buf, xml->value);
        break;
        
    case EJS_XML_TEXT:
        mprPutStringToBuf(buf, xml->value);
        break;
    }
    
    node->var.visited = 0;
    
    return 0;
}



static void indent(MprBuf *bp, int level)
{
    int     i;

    for (i = 0; i < level; i++) {
        mprPutCharToBuf(bp, '\t');
    }
}


#else
void __ejsXMLLoaderDummy() {}
#endif /* BLD_FEATURE_EJS_E4X */


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../types/xml/ejsXMLLoader.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsException.c"
 */
/************************************************************************/

/**
 *  ejsException.c - Error Exception class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Throw an exception.
 */
EjsVar *ejsThrowException(Ejs *ejs, EjsVar *obj)
{
    mprAssert(obj);

    /*
     *  Set ejs->exception. The VM will notice this and initiate exception handling
     */
    ejs->exception = obj;
    ejs->attention = 1;
    return obj;
}


/*
 *  Create an exception object.
 */
static EjsVar *createException(Ejs *ejs, EjsType *type, cchar* fmt, va_list fmtArgs)
{
    EjsVar          *error, *argv[1];
    char            *msg;

    mprAssert(type);

    if (ejs->noExceptions) {
        return 0;
    }

    mprAllocVsprintf(ejs, &msg, MPR_MAX_STRING, fmt, fmtArgs);
    argv[0] = (EjsVar*) ejsCreateString(ejs, msg);

    if (argv[0] == 0) {
        mprAssert(argv[0]);
        return 0;
    }

    error = (EjsVar*) ejsCreateInstance(ejs, type, 1, argv);
    if (error == 0) {
        mprAssert(error);
        return 0;
    }
    mprFree(msg);

    return error;
}


EjsVar *ejsCreateException(Ejs *ejs, int slot, cchar *fmt, va_list fmtArgs)
{
    EjsType     *type;
    EjsVar      *error;
    char        *buf;

#if DEBUG_IDE
    mprBreakpoint(0, 0);
#endif

    mprAssert(ejs->exception == 0);

    if (ejs->exception) {
        mprAllocVsprintf(ejs, &buf, 0, fmt, fmtArgs);
        mprError(ejs, "Double exception: %s", buf);
        return ejs->exception;
    }

    if (!ejs->initialized || (ejs->flags & EJS_FLAG_EMPTY)) {
        mprAllocVsprintf(ejs, &buf, 0, fmt, fmtArgs);
        mprError(ejs, "Exception: %s", buf);
        return ejs->exception;
    }

    type = (EjsType*) ejsGetProperty(ejs, ejs->global, slot);

    if (type == 0) {
        type = ejs->errorType;
    }

    error = createException(ejs, type, fmt, fmtArgs);

    if (error) {
        ejsThrowException(ejs, error);
    }
    return error;
}


EjsVar *ejsThrowArgError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_ArgError, fmt, fmtArgs);
}


EjsVar *ejsThrowArithmeticError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_ArithmeticError, fmt, fmtArgs);
}


EjsVar *ejsThrowAssertError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_AssertError, fmt, fmtArgs);
}


EjsVar *ejsThrowInstructionError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_InstructionError, fmt, fmtArgs);
}


EjsVar *ejsThrowError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_Error, fmt, fmtArgs);
}


EjsVar *ejsThrowIOError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_IOError, fmt, fmtArgs);
}


EjsVar *ejsThrowInternalError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_InternalError, fmt, fmtArgs);
}


EjsVar *ejsThrowMemoryError(Ejs *ejs)
{
    /*
     *  Don't do double exceptions for memory errors
     */
    if (ejs->exception == 0) {
        return ejsCreateException(ejs, ES_MemoryError, 0, 0);
    }
    return ejs->exception;
}


EjsVar *ejsThrowOutOfBoundsError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_OutOfBoundsError, fmt, fmtArgs);
}


EjsVar *ejsThrowReferenceError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_ReferenceError, fmt, fmtArgs);
}


EjsVar *ejsThrowResourceError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_ResourceError, fmt, fmtArgs);
}


EjsVar *ejsThrowStateError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_StateError, fmt, fmtArgs);
}


EjsVar *ejsThrowSyntaxError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_SyntaxError, fmt, fmtArgs);
}


EjsVar *ejsThrowTypeError(Ejs *ejs, cchar *fmt, ...)
{
    va_list     fmtArgs;

    if (fmt) {
        va_start(fmtArgs, fmt);
    }
    return ejsCreateException(ejs, ES_TypeError, fmt, fmtArgs);
}


/*
 *  Format the stack backtrace
 */
char *ejsFormatStack(Ejs *ejs)
{
    EjsFrame        *frame;
    EjsType         *type;
    EjsFunction     *fun;
    cchar           *typeName, *functionName, *line, *typeSep, *codeSep;
    char            *backtrace, *traceLine;
    int             level, len;

    mprAssert(ejs);

    backtrace = 0;
    len = 0;
    level = 0;

    for (frame = ejs->frame; frame; frame = frame->caller) {

        if (frame->currentLine == 0) {
            line = "";
        } else {
            for (line = frame->currentLine; *line && isspace((int) *line); line++) {
                ;
            }
        }

        typeName = "";
        functionName = "global";

        fun = &frame->function;
        if (fun) {
            if (fun->owner) {
                functionName = ejsGetPropertyName(ejs, fun->owner, fun->slotNum).name;
            }
            if (ejsIsType(fun->owner)) {
                type = (EjsType*) fun->owner;
                if (type) {
                    typeName = type->qname.name;
                }
            }
        }
        typeSep = (*typeName) ? "." : "";
        codeSep = (*line) ? "->" : "";

        if (mprAllocSprintf(ejs, &traceLine, MPR_MAX_STRING, " [%02d] %s, %s%s%s, line %d %s %s\n",
                level++, frame->fileName ? frame->fileName : "script", typeName, typeSep, functionName,
                frame->lineNumber, codeSep, line) < 0) {
            break;
        }
        backtrace = (char*) mprRealloc(ejs, backtrace, len + (int) strlen(traceLine) + 1);
        if (backtrace == 0) {
            return 0;
        }
        memcpy(&backtrace[len], traceLine, strlen(traceLine) + 1);
        len += (int) strlen(traceLine);
        mprFree(traceLine);
    }
    return backtrace;
}


/*
 *  Public routine to set the error message. Caller MUST NOT free.
 */
char *ejsGetErrorMsg(Ejs *ejs, int withStack)
{
    EjsVar      *message, *stack, *error;
    cchar       *name;
    char        *buf;

    if (ejs->flags & EJS_FLAG_EMPTY) {
        return "";
    }

    error = (EjsVar*) ejs->exception;
    message = stack = 0;
    name = 0;

    if (error) {
        name = error->type->qname.name;

        if (ejsIsA(ejs, error, ejs->errorType)) {
            message = ejsGetProperty(ejs, error, ES_Error_message);
            stack = ejsGetProperty(ejs, error, ES_Error_stack);

        } else if (ejsIsString(error)) {
            name = "Details";
            message = error;

        } else if (error == (EjsVar*) ejs->stopIterationType) {
            name = "StopIteration";
            message = (EjsVar*) ejsCreateString(ejs, "Uncaught StopIteration exception");
        }
    }
    if (!withStack) {
        stack = 0;
    }

    if (stack && ejsIsString(stack) && message && ejsIsString(message)){
        mprAllocSprintf(ejs, &buf, 0, "%s Exception: %s\nStack:\n%s", name, ((EjsString*) message)->value, 
            ((EjsString*) stack)->value);

    } else if (message && ejsIsString(message)){
        mprAllocSprintf(ejs, &buf, 0, "%s: %s", name, ((EjsString*) message)->value);

    } else {
        if (error) {
            buf = mprStrdup(ejs, "Unknown exception object type");
        } else {
            buf = mprStrdup(ejs, "");
        }
    }

    mprFree(ejs->errorMsg);
    ejs->errorMsg = buf;

    return buf;
}


bool ejsHasException(Ejs *ejs)
{
    return ejs->exception != 0;
}


EjsVar *ejsGetException(Ejs *ejs)
{
    return ejs->exception;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsException.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsGarbage.c"
 */
/************************************************************************/

/**
 *  ejsGarbage.c - EJS Garbage collector.
 *
 *  This implements a non-compacting, generational mark and sweep collection algorithm.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static void addRoot(Ejs *ejs, int generation, EjsVar *obj);
static inline void addVar(struct Ejs *ejs, struct EjsVar *vp, int generation);
static inline void linkVar(EjsGen *gen, EjsVar *vp);
static void mark(Ejs *ejs, int generation);
static void markFrame(Ejs *ejs, EjsFrame *frame);
static void markGlobal(Ejs *ejs);
static inline bool memoryUsageOk(Ejs *ejs);
static inline void pruneTypePools(Ejs *ejs);
static inline void moveGen(Ejs *ejs, EjsVar *vp, EjsVar *prev, int oldGen, int newGen);
static void resetRoots(Ejs *ejs, int generation);
static int sweep(Ejs *ejs, int generation);
static inline void unlinkVar(EjsGen *gen, EjsVar *prev, EjsVar *vp);

#if BLD_DEBUG
static void checkMarks(Ejs *ejs);
#endif

#if BLD_DEBUG || 1
static void *ejsBreakAddr = (void*) 0x555dd8;
static void checkAddr(EjsVar *addr) {
    if ((void*) addr == ejsBreakAddr) { 
        addr = ejsBreakAddr;
    }
}
#else
#define checkAddr(addr)
#endif

/*
 *  Create the GC service
 */
int ejsCreateGCService(Ejs *ejs)
{
    EjsGC       *gc;
    EjsGen      *gen;
    int         i;

    mprAssert(ejs);

    gc = &ejs->gc;
    gc->enabled = !(ejs->flags & EJS_FLAG_COMPILER);
    gc->enableIdleCollect = 1;
    gc->enableDemandCollect = 1;
    gc->workQuota = EJS_GC_WORK_QUOTA;
    gc->firstGlobal = ES_global_NUM_CLASS_PROP;

    /*
     *  Start in the eternal generation for all builtin types. ejsSetGeneration will be called later to set to the NEW gen.
     */
    gc->allocGeneration = EJS_GEN_ETERNAL;

    gc->pools = mprAllocZeroed(ejs, sizeof(EjsPool) * EJS_MAX_TYPE);
    if (gc->pools == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    gc->numPools = EJS_MAX_TYPE;

    /*
     *  Allocate space for the cross generational root links
     */
    for (i = 0; i < EJS_MAX_GEN; i++) {
        gen = &gc->generations[i];
        gen->roots = mprAllocZeroed(ejs, sizeof(EjsVar*) * EJS_NUM_CROSS_GEN);
        if (gen->roots == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        gen->nextRoot = gen->roots;
        gen->peakRoot = gen->roots;
        gen->lastRoot = &gen->roots[EJS_NUM_CROSS_GEN - 1];
    }
    return 0;
}


/*
 *  Collect the garbage. This is a mark and sweep over all possible objects. If an object is not referenced, it and 
 *  all contained properties will be freed. Collection is done in generations.
 */
void ejsCollectGarbage(Ejs *ejs, int mode)
{
    EjsGC       *gc;
    int         generation, i, count, totalCreated, prevMax;
    
    gc = &ejs->gc;

    if (!gc->enabled || gc->collecting || !ejs->initialized) {
        return;
    }
    gc->collecting = 1;
    gc->totalSweeps++;

    /*
     *  Collecting a generation implicitly collects all younger generations. If collecting all, just start at old.
     *  If any cross-generational root storage has overflowed, we must do a full GC.
     */
    if (mode == EJS_GC_ALL || gc->overflow) {
        gc->overflow = 0;
        generation = EJS_GEN_OLD;
        mark(ejs, generation);
        sweep(ejs, generation);
        
    } else if (mode == EJS_GC_QUICK) {
        generation = EJS_GEN_NEW;
        mark(ejs, generation);
        sweep(ejs, generation);
        
    } else {
        
        /*
         *  Smart collection. Find the oldest generation worth examining. The sweep may promote objects to older generations,
         *  so keep collecting any worthwhile generations.
         */
        while (1) {
            /*
             */
            generation = 0;
            prevMax = 0;
            totalCreated = 0;
            for (i = EJS_GEN_NEW; i < EJS_GEN_ETERNAL; i++) {
                count = gc->generations[i].newlyCreated;
                if (count > prevMax || count > EJS_GC_WORK_QUOTA) {
                    prevMax = count;
                    generation = i;
                }
                totalCreated += count;
            }

            /*
             *  Collect from this generation and all younger generations.
             */
            if (totalCreated < EJS_GC_WORK_QUOTA) {
                break;
            }
            mark(ejs, generation);
            sweep(ejs, generation);
        }
    }

    if (!memoryUsageOk(ejs)) {
        pruneTypePools(ejs);
    }

    gc->workDone = 0;
    gc->collecting = 0;
    ejs->gc.required = 0;
}


/*
 *  Mark phase. Mark objects that are still in use and should not be collected.
 */
static void mark(Ejs *ejs, int generation)
{
    EjsVar      *vp, **sp, **lim, **src;
    EjsFrame    *frame;
    EjsGC       *gc;
    EjsModule   *mp;
    EjsGen      *gen;
    int         next, i;

    gc = &ejs->gc;
    gc->collectGeneration = generation;

#if BLD_DEBUG
    mprLog(ejs, 6, "\nGC: Marked Blocks: generation %d", generation);
#endif

    markGlobal(ejs);

    if (ejs->result) {
        ejsMarkVar(ejs, NULL, ejs->result);
    }
    if (ejs->exception) {
        ejsMarkVar(ejs, NULL, ejs->exception);
    }

    /*
     *  Mark initializers
     */
    for (next = 0; (mp = (EjsModule*) mprGetNextItem(ejs->modules, &next)) != 0;) {
        if (mp->initializer /* TODO MOB TEMP && !mp->initialized */) {
            ejsMarkVar(ejs, NULL, (EjsVar*) mp->initializer);
        }
    }

    /*
     *  Mark each frame. This includes all stack slots, ie. (local vars, arguments and expression temporaries)
     */
    lim = ejs->stack.top + 1;
    for (frame = ejs->frame; frame; frame = frame->prev) {
        markFrame(ejs, frame);
        for (sp = frame->stackBase; sp < lim; sp++) {
            vp = *sp;
            if (vp) {
                ejsMarkVar(ejs, NULL, vp);
            }
        }
        lim = frame->prevStackTop + 1;
    }

    /*
     *  Mark the cross-generational roots. Compact and remove old roots in the process. Must now traverse all objects
     *  that are referenced from these roots - so set the collectGeneration to eternal.
     */
    gc->collectGeneration = EJS_GEN_ETERNAL;
    for (i = 0; i <= generation; i++) {
        gen = &gc->generations[i];
        for (src = gen->roots; src < gen->nextRoot; src++) {
            ejsMarkVar(ejs, NULL, *src);
        }
    }
}


/*
 *  Sweep up the garbage for a given generation
 */
static int sweep(Ejs *ejs, int maxGeneration)
{
    EjsVar      *vp, *next, *prev;
    EjsGC       *gc;
    EjsGen      *gen;
    int         total, count, aliveCount, generation, i;

    gc = &ejs->gc;
    
    total = 0;
    count = 0;
    aliveCount = 0;

    /*
     *  Must go from oldest to youngest generation incase moving objects to elder generations and we clear the mark. Must
     *  not re-examine.
     */
    for (generation = maxGeneration; generation >= 0; generation--) {

        count = 0;
        gc->collectGeneration = generation;

        /*
         *  Traverse all objects in the required generation
         */
        gen = &gc->generations[generation];
        for (prev = 0, vp = gen->next; vp; vp = next) {

            next = vp->next;
            checkAddr(vp);
            mprAssert(vp->generation == generation);

            if (vp->marked) {
                /*
                 *  In use and surviving at least one collection cycle. Move to a new generation if it has survived 2 cycles.
                 */
                if (vp->generation < EJS_GEN_OLD) {
                    if (vp->survived) {
                        vp->survived = 0;
                        moveGen(ejs, vp, prev, vp->generation, vp->generation + 1);

                    } else {
                        vp->survived = 1;
                        prev = vp;
                    }
                
                } else {
                    prev = vp;
                }
                vp->marked = 0;
                aliveCount++;
                continue;

            } else if (vp->permanent) {
                prev = vp;
                aliveCount++;
                continue;
            }

            unlinkVar(gen, prev, vp);
            if (vp->type->hasFinalizer) {
                ejsFinalizeVar(ejs, vp);
            }
            ejsDestroyVar(ejs, vp);
            count++;
        }

        gc->allocatedObjects -= count;
        gc->totalReclaimed += count;
        gen->totalReclaimed += count;
        gen->totalSweeps++;
        gen->newlyCreated = 0;

        total += count;

    }

    for (i = maxGeneration; i < EJS_MAX_GEN; i++) {
        gen = &gc->generations[i];
        for (vp = gen->next; vp; vp = vp->next) {
            if (vp->marked) {
                vp->marked = 0;
            }
        }
    }
    
    resetRoots(ejs, maxGeneration);

#if BLD_DEBUG
    mprLog(ejs, 6, "GC: Sweep freed %d objects, alive %d", total, aliveCount);
    checkMarks(ejs);
#endif
    return total;
}


/*
 *  Compact the cross generation root objects.
 */
static void resetRoots(Ejs *ejs, int generation)
{
    EjsFrame    *frame;
    EjsVar      *vp, **src, **dest;
    EjsGen      *gen;
    int         i, j;

    for (frame = ejs->frame; frame; frame = frame->prev) {
        frame->function.block.obj.var.marked = 0;
    }

    ejs->gc.collectGeneration = EJS_GEN_ETERNAL;
    for (i = 0; i <= generation; i++) {
        gen = &ejs->gc.generations[i];
        for (src = dest = gen->roots; src < gen->nextRoot; ) {
            vp = *src;
            checkAddr(vp);

            if (vp->rootLinks & (1 << i)) {
                *dest++ = *src++;

            } else {
                vp->rootLinks &= ~(1 << i);
                vp->refLinks &= ~(1 << i);
                if (vp->refLinks) {
                    for (j = i + 1; j <= generation; j++) {
                        if (vp->refLinks & (1 << j)) {
                            addRoot(ejs, j, *src);
                            break;
                        }
                    }
                }
                src++;
            }
        }
        gen->nextRoot = dest;
        gen->rootCount = gen->nextRoot - gen->roots;
        *gen->nextRoot = 0;
    }
}


#if BLD_DEBUG
static void checkMarks(Ejs *ejs)
{
    EjsVar      *vp;
    EjsGen      *gen;
    int         i;

    ejs->gc.collectGeneration = EJS_GEN_ETERNAL;
    for (i = 0; i < EJS_MAX_GEN; i++) {
        gen = &ejs->gc.generations[i];
        for (vp = gen->next; vp; vp = vp->next) {
            mprAssert(!vp->marked);
            mprAssert(vp->magic != 0xf1f1f1f1);
        }
    }
}
#endif


static void markFrame(Ejs *ejs, EjsFrame *frame)
{
    EjsBlock    *block;
    int         next;

    if (frame->returnValue) {
        ejsMarkVar(ejs, NULL, frame->returnValue);
    }
    if (frame->saveException) {
        ejsMarkVar(ejs, NULL, frame->saveException);
    }
    if (frame->saveFrame) {
        markFrame(ejs, frame->saveFrame);
    }    
    if (frame->thisObj) {
        ejsMarkVar(ejs, NULL, frame->thisObj);
    } 

    if (frame->needClosure.length > 0) {
        next = 0;
        while ((block = ejsGetNextItem(&frame->needClosure, &next)) != 0) {
            ejsMarkVar(ejs, NULL, (EjsVar*) block);
        }
    }

    ejsMarkVar(ejs, NULL, (EjsVar*) &frame->function);
}


static void markGlobal(Ejs *ejs)
{
    EjsGC       *gc;
    EjsVar      *vp;
    EjsObject   *obj;
    int         i, eternalSoFar;

    gc = &ejs->gc;

    obj = (EjsObject*) ejs->global;
    obj->var.marked = 1;
    eternalSoFar = 0;

    for (i = gc->firstGlobal; i < obj->numProp; i++) {
        vp = obj->slots[i];
        if (vp->generation == EJS_GEN_ETERNAL) {
            if (eternalSoFar) {
                gc->firstGlobal = i;
            }
            continue;
        }
        eternalSoFar = 0;
        if (vp == 0 || vp == ejs->nullValue) {
            continue;
        }
        ejsMarkVar(ejs, NULL, vp);
    }
}


/*
 *  Mark a variable as used. All variable marking comes through here.
 */
void ejsMarkVar(Ejs *ejs, EjsVar *container, EjsVar *vp)
{  
    if (vp->marked) {
        return;
    }
    
    /*
     *  Don't traverse generations older than the one being marked. 
     */
    if (vp->generation <= ejs->gc.collectGeneration) {
        checkAddr(vp);
        vp->marked = 1;
        if (container) {
            if (vp->generation < EJS_GEN_ETERNAL) {
                container->refLinks |= (1 << vp->generation);
            }
        }
        (vp->type->helpers->markVar)(ejs, container, vp);
    }
}


static inline bool memoryUsageOk(Ejs *ejs)
{
    MprAlloc    *alloc;
    uint        memory;

    memory = mprGetUsedMemory(ejs);
    alloc = mprGetAllocStats(ejs);
    return memory < alloc->redLine;
}


static inline void pruneTypePools(Ejs *ejs)
{
    EjsPool     *pool;
    MprAlloc    *alloc;
    EjsGC       *gc;
    EjsVar      *vp, *nextVp;
    uint        memory;
    int         i;

    gc = &ejs->gc;

    /*
     *  Still insufficient memory, must reclaim all objects from the type pools.
     */
    for (i = 0; i < gc->numPools; i++) {
        pool = &gc->pools[i];
        if (pool->count) {
            for (vp = pool->next; vp; vp = nextVp) {
                nextVp = vp->next;
                mprFree(vp);
            }
            pool->count = 0;
        }
    }
    gc->totalRedlines++;

    memory = mprGetUsedMemory(ejs);
    alloc = mprGetAllocStats(ejs);

    if (memory >= alloc->maxMemory) {
        /*
         *  Could not provide sufficient memory. Go into graceful degrade mode
         */
        ejsThrowMemoryError(ejs);
        ejsGracefulDegrade(ejs);
    }
}


void ejsMakePermanent(Ejs *ejs, EjsVar *vp)
{
    vp->permanent = 1;
}


void ejsMakeTransient(Ejs *ejs, EjsVar *vp)
{
    vp->permanent = 0;
}


/*
 *  Allocate a new variable. Size is set to the extra bytes for properties in addition to the type's instance size.
 */
EjsVar *ejsAllocVar(Ejs *ejs, EjsType *type, int extra)
{
    EjsPool     *pool;
    EjsGC       *gc;
    EjsVar      *vp;
    uint        size;
    int         generation;
#if BLD_DEBUG
    int         seqno;
#endif

    mprAssert(ejs);
    mprAssert(type);

    gc = &ejs->gc;
    generation = gc->allocGeneration;

    if (0 <= type->id && type->id < gc->numPools) {
        pool = &gc->pools[type->id];
        vp = (EjsVar*) pool->next;
        if (type == ejs->typeType) {
            generation = EJS_GEN_ETERNAL;
        }

    } else {
        vp = 0;
        pool = 0;
    }

    if (vp) {
        pool->next = vp->next;
        pool->reuse++;
        pool->count--;
        mprAssert(pool->count >= 0);

#if BLD_DEBUG
        seqno = vp->seqno;
        memset(vp, 0, type->instanceSize);
        vp->type = type;
        vp->seqno = seqno;
#else
        memset(vp, 0, type->instanceSize);
        vp->type = type;
#endif
        
    } else {
        /*
         *  TODO - remove zeroed
         */
        size = max(1, type->numAlloc) * (extra + type->instanceSize);
        vp = (EjsVar*) mprAllocZeroed(ejs, size);
        if (vp == 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
        vp->type = type;
        vp->next = 0;

        /*
         *  TODO OPT - bitfields
         */
        vp->generation = 0;
        vp->rootLinks = 0;
        vp->refLinks = 0;
        vp->builtin = 0;
        vp->dynamic = 0;
        vp->hasGetterSetter = 0;
        vp->isFunction = 0;
        vp->isObject = 0;
        vp->isInstanceBlock = 0;
        vp->isType = 0;
        vp->isFrame = 0;
        vp->hidden = 0;
        vp->marked = 0;
        vp->native = 0;
        vp->nativeProc = 0;
        vp->permanent = 0;
        vp->survived = 0;
        vp->visited = 0;
    }

    addVar(ejs, vp, generation);

    if (pool) {
        pool->type = type;
        pool->allocated++;
        if (pool->allocated > pool->peakAllocated) {
            pool->peakAllocated = pool->allocated;
        }
    }

#if BLD_DEBUG
    mprAssert(vp->magic != EJS_GC_IN_USE);
    vp->magic = EJS_GC_IN_USE;
    vp->seqno++;
#endif

    return (EjsVar*) vp;
}


/*
 *  Free a variable. This is should only ever be called by the destroyVar helpers to free or recycle the object to a type 
 *  specific free pool. Users should let the GC discovery unused objects which will then call ejsDestroyObject when an object
 *  is no longer referenced.
 */
void ejsFreeVar(Ejs *ejs, EjsVar *vp)
{
    EjsType     *type;
    EjsGC       *gc;
    EjsPool     *pool;

    mprAssert(vp);
    mprAssert(vp->next == 0);
    
#if BLD_DEBUG
    mprAssert(vp->magic == EJS_GC_IN_USE);
    vp->magic = EJS_GC_FREE;
#endif

    type = vp->type;

#if BLD_FEATURE_MEMORY_DEBUG
    {
        int seqno = vp->seqno;
        memset(vp, 0xf4, type->instanceSize);
        vp->seqno = seqno;
    }
#else
    //  TODO - MOB - TEMP
    memset(vp, 0xf4, type->instanceSize);
#endif

    /*
     *  Return the object to the type pool
     */
    gc = &ejs->gc;
    if (type->id >= 0 && type->id < gc->numPools) {
        pool = &gc->pools[type->id];
        vp->next = pool->next;
        pool->next = (EjsVar*) vp;

        /*
         *  Update stats
         */
        pool->allocated--;
        pool->count++;
        if (pool->count > pool->peakCount) {
            pool->peakCount = pool->count;
        }

    } else {
        mprFree(vp);
    }
}


static inline void linkVar(EjsGen *gen, EjsVar *vp)
{
    mprAssert(gen);
    mprAssert(vp);
    mprAssert(vp->next == 0);

    vp->next = gen->next;
    gen->next = vp;

#if BLD_DEBUG
    gen->inUse++;
#endif
}


static inline void unlinkVar(EjsGen *gen, EjsVar *prev, EjsVar *vp)
{
    mprAssert(gen);
    mprAssert(vp);

    if (prev) {
        prev->next = vp->next;
    } else {
        gen->next = vp->next;
    }
#if BLD_DEBUG
    gen->inUse--;
    mprAssert(gen->inUse >= 0);
    vp->next = 0;
#endif
}


static inline void addVar(Ejs *ejs, EjsVar *vp, int generation)
{
    EjsGC       *gc;
    EjsGen      *gen;

    gc = &ejs->gc;

    mprAssert(gc);
    mprAssert(vp);
    mprAssert(0 <= generation && generation <= EJS_GEN_ETERNAL);

    if (vp == 0) {
        return;
    }
    checkAddr(vp);

    vp->generation = generation;
    vp->rootLinks = 0;
    vp->refLinks = 0;

    gen = &gc->generations[generation];
    linkVar(gen, vp);

    /*
     *  Update GC stats
     */
    gen->newlyCreated++;
    gc->totalAllocated++;
    gc->allocatedObjects++;
    if (gc->allocatedObjects >= gc->peakAllocatedObjects) {
        gc->peakAllocatedObjects = gc->allocatedObjects;
    }

    if (vp->type == ejs->typeType) {
        gc->allocatedTypes++;
        if (gc->allocatedTypes >= gc->peakAllocatedTypes) {
            gc->peakAllocatedTypes = gc->allocatedTypes;
        }
    }

    if (++gc->workDone >= gc->workQuota) {
        ejs->gc.required = 1;
        ejs->attention = 1;
        gc->workDone = 0;
    }
}


/*
 *  Move a var to a new, older generation
 */
static inline void moveGen(Ejs *ejs, EjsVar *vp, EjsVar *prev, int oldGen, int newGen)
{
    EjsGC       *gc;
    EjsGen      *nextGen;
    
    mprAssert(vp);
    mprAssert(oldGen < newGen);
    
    gc = &ejs->gc;
    nextGen = &gc->generations[newGen];

    unlinkVar(&gc->generations[oldGen], prev, vp);
    linkVar(nextGen, vp);
    nextGen->newlyCreated++;    

    addRoot(ejs, oldGen, vp);
    vp->generation = newGen;
}


/*
 *  Return true if there is time to do a garbage collection and if we will benefit from it.
 *  TODO - this is currently not called.
 */
int ejsIsTimeForGC(Ejs *ejs, int timeTillNextEvent)
{
    EjsGC       *gc;

    if (timeTillNextEvent < EJS_MIN_TIME_FOR_GC) {
        /*
         *  This is a heuristic where we want a good amount of idle time so that a proactive garbage collection won't 
         *  delay any I/O events.
         */
        return 0;
    }

    /*
     *  Return if we haven't done enough work to warrant a collection Trigger a little short of the work quota to try to run 
     *  GC before a demand allocation requires it.
     */
    gc = &ejs->gc;
    if (!gc->enabled || !gc->enableIdleCollect || gc->workDone < (gc->workQuota - EJS_GC_MIN_WORK_QUOTA)) {
        return 0;
    }

    mprLog(ejs, 6, "Time for GC. Work done %d, time till next event %d", gc->workDone, timeTillNextEvent);

    return 1;
}


void ejsEnableGC(Ejs *ejs, bool on)
{
    ejs->gc.enabled = on;
}


/*
 *  On a memory allocation failure, go into graceful degrade mode. Set all slab allocation chunk increments to 1 
 *  so we can create an exception block to throw.
 */
void ejsGracefulDegrade(Ejs *ejs)
{
    mprLog(ejs, 1, "WARNING: Memory almost depleted. In graceful degrade mode");

    //  TODO -- need to notify MPR slabs to allocate in chunks of 1
    ejs->gc.degraded = 1;
    mprSignalExit(ejs);
}


/*
 *  Set a GC reference from object to value, in response to: "obj->field = value"
 */
void ejsSetReference(Ejs *ejs, EjsVar *obj, EjsVar *value)
{
    if (value && value->generation < obj->generation) {

        mprAssert(value->generation < EJS_GEN_ETERNAL);

        //  TODO - this won't work with master interpreters.
        //  mprAssert(!obj->master);
        
        if ((obj->rootLinks & (1 << value->generation)) == 0) {
            addRoot(ejs, value->generation, obj);
        }
    }
}


static void addRoot(Ejs *ejs, int generation, EjsVar *obj)
{
    EjsGen  *gen;

    mprAssert(obj);
    mprAssert(obj->type);
    mprAssert(0 <= generation && generation <= EJS_GEN_ETERNAL);
    
    if (obj->isFrame) {
        return;
    }
    obj->rootLinks |= (1 << generation);
    obj->refLinks |= (1 << generation);

    gen = &ejs->gc.generations[generation];
    if (gen->nextRoot < gen->lastRoot) {
        *gen->nextRoot++ = obj;
        *gen->nextRoot = 0;
        if (gen->nextRoot > gen->peakRoot) {
            gen->peakRoot = gen->nextRoot;
        }

    } else {
        ejs->gc.overflow = 1;
        ejs->gc.totalOverflows++;
    }
}


int ejsSetGeneration(Ejs *ejs, int generation)
{
    int     old;
    
    old = ejs->gc.allocGeneration;
    ejs->gc.allocGeneration = generation;
    return old;
}


void ejsPrintAllocReport(Ejs *ejs)
{
    EjsType         *type;
    EjsGC           *gc;
    EjsGen          *gen;
    EjsPool         *pool;
    MprAlloc        *ap;
    int             i, maxSlot, typeMemory;

    gc = &ejs->gc;
    ap = mprGetAllocStats(ejs);
    
    /*
     *  EJS stats
     */
    mprLog(ejs, 0, "\n\nEJS Memory Statistics");
    mprLog(ejs, 0, "  Types allocated        %,14d", gc->allocatedTypes / 2);
    mprLog(ejs, 0, "  Objects allocated      %,14d", gc->allocatedObjects);
    mprLog(ejs, 0, "  Peak objects allocated %,14d", gc->peakAllocatedObjects);

    /*
     *  Per type
     */
    mprLog(ejs, 0, "\nObject Cache Statistics");
    mprLog(ejs, 0, "------------------------");
    mprLog(ejs, 0, "Name                TypeSize  ObjectSize  ObjectCount  PeakCount  FreeList  PeakFreeList   ReuseCount");
    
    maxSlot = ejsGetPropertyCount(ejs, ejs->global);
    typeMemory = 0;

    for (i = 0; i < gc->numPools; i++) {
        pool = &ejs->gc.pools[i];
        type = pool->type;
        if (type == 0 || !ejsIsType(type)) {
            continue;
        }

        if (type->id < 0 || type->id >= gc->numPools) {
            continue;
        }

        pool = &ejs->gc.pools[type->id];

        mprLog(ejs, 0, "%-22s %,5d %,8d %,10d  %,10d, %,9d, %,10d, %,14d", type->qname.name, ejsGetTypeSize(ejs, type), 
            type->instanceSize, pool->allocated, pool->peakAllocated, pool->count, pool->peakCount, pool->reuse);

#if FUTURE
        mprLog(ejs, 0, "  Current object memory  %,14d K", / 1024);
        mprLog(ejs, 0, "  Peak object memory     %,14d K", / 1024);
#endif
        typeMemory += ejsGetTypeSize(ejs, type);
    }
    mprLog(ejs, 0, "\nTotal type memory        %,14d K", typeMemory / 1024);


    mprLog(ejs, 0, "\nEJS Garbage Collector Statistics");
    mprLog(ejs, 0, "  Total allocations      %,14d", gc->totalAllocated);
    mprLog(ejs, 0, "  Total reclaimations    %,14d", gc->totalReclaimed);
    mprLog(ejs, 0, "  Total sweeps           %,14d", gc->totalSweeps);
    mprLog(ejs, 0, "  Total redlines         %,14d", gc->totalRedlines);
    mprLog(ejs, 0, "  Total overflows        %,14d", gc->totalOverflows);

    mprLog(ejs, 0, "\nGC Generation Statistics");
    for (i = 0; i < EJS_MAX_GEN; i++) {
        gen = &gc->generations[i];
        mprLog(ejs, 0, "  Generation %d", i);
        mprLog(ejs, 0, "    Newly created        %,14d", gen->newlyCreated);
        mprLog(ejs, 0, "    Objects in-use       %,14d", gen->inUse);
        mprLog(ejs, 0, "    Total reclaimations  %,14d", gen->totalReclaimed);
        mprLog(ejs, 0, "    Total sweeps         %,14d", gen->totalSweeps);
        mprLog(ejs, 0, "    Peak root usage      %,14d", gen->peakRoot - gen->roots);
    }

    mprLog(ejs, 0, "  Object GC work quota   %,14d", gc->workQuota);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsGarbage.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsInterp.c"
 */
/************************************************************************/

/*
 *  ejsInterp.c - Virtual Machine Interpreter for Ejscript.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  The stack is a stack of pointers to EjsVar. The top of stack (stack.top) always points to the current top item 
 *  on the stack. To push a new value, top is incremented then the value is stored. To pop, simply copy the value at 
 *  top and decrement top ptr.
 */
#define pop(ejs)            *ejs->stack.top--
#define push(ejs, value)    (*(++(ejs->stack.top))) = ((EjsVar*) (value))
#define popString(ejs)      ((EjsString*) pop(ejs))
#define getType(frame)      ((EjsType*) getGlobalArg(frame))

//  TODO - If PUT_PROPERTY validated the type on assignment, we could eliminate testing against numProp and 
//  TODO - vp should never be zero
//  TODO - OPTIMIZE GET_PROPERTY

#define GET_PROPERTY(_ejs, _thisObj, _vp, _slotNum)                                                         \
    if (unlikely(_vp == 0 || (EjsVar*) (_vp) == (EjsVar*) ejs->nullValue)) {                                \
        goto nullException;                                                                                 \
    } else {                                                                                                \
        EjsVar *_value;                                                                                     \
        if (ejsIsObject(_vp)) {                                                                             \
            if (unlikely((_slotNum >= ((EjsObject*)(_vp))->numProp))) {                                     \
                goto slotReferenceException;                                                                \
            }                                                                                               \
            _value = ((EjsObject*)(_vp))->slots[_slotNum];                                                  \
        } else {                                                                                            \
            _value = ejsGetProperty(_ejs, (EjsVar*) (_vp), _slotNum);                                       \
        }                                                                                                   \
        mprAssert(_value);                                                                                  \
        if (unlikely(_value->isFunction)) {                                                                 \
            frame = getFunction(_ejs, _thisObj, (EjsVar*)(_vp), _slotNum, (EjsFunction*) _value, &local);   \
        } else {                                                                                            \
            push(_ejs, _value);                                                                             \
        }                                                                                                   \
    }

#define PUT_PROPERTY(_ejs, _thisObj, _vp, _slotNum)                                                         \
    if (unlikely(_vp == 0 || (EjsVar*) (_vp) == (EjsVar*) ejs->nullValue)) {                                \
        goto nullException;                                                                                 \
    } else if (_slotNum >= ((EjsObject*)(_vp))->numProp) {                                                  \
        ejsSetProperty(_ejs, (EjsVar*) (_vp), _slotNum, pop(_ejs));                                         \
    } else {                                                                                                \
        EjsVar  *_oldValue;                                                                                 \
        if (ejsIsObject(_vp)) {                                                                             \
            _oldValue = ((EjsObject*)(_vp))->slots[_slotNum];                                               \
            mprAssert(_oldValue);                                                                           \
        } else {                                                                                            \
            _oldValue = ejsGetProperty(_ejs, (EjsVar*) (_vp), _slotNum);                                    \
        }                                                                                                   \
        if (unlikely(_oldValue->isFunction)) {                                                              \
            EjsFunction *_fun = (EjsFunction*) _oldValue;                                                   \
            if (_fun->getter && _fun->nextSlot) {                                                           \
                putFunction(_ejs, (EjsVar*) (_thisObj ? _thisObj : _vp), _fun, pop(_ejs));                  \
            }                                                                                               \
        } else {                                                                                            \
            EjsVar *_value = pop(_ejs);                                                                     \
            if (ejsIsObject(_vp)) {                                                                         \
                ((EjsObject*)(_vp))->slots[_slotNum] = _value;                                              \
            } else {                                                                                        \
                ((EjsObject*)(_vp))->slots[_slotNum] = _value;                                              \
            }                                                                                               \
            ejsSetReference(_ejs, (EjsVar*) _vp, (EjsVar*) _value);                                         \
        }                                                                                                   \
    }

#if LINUX || MACOSX || LINUX || SOLARIS || VXWORKS
    #define CASE(opcode) opcode
    #define BREAK \
        if (1) { \
            opcode = getByte(frame); \
            goto *opcodeJump[traceCode(ejs, opcode)]; \
        } else
    #define CHECK \
        if (1) { \
            if (unlikely(ejs->attention)) { \
                if ((frame = payAttention(ejs)) == 0) { \
                    return; \
                } \
                local = (EjsObject*) frame->currentFunction; \
            } \
        } else
#else
    /*
     *  Traditional switch for compilers (looking at you MS) without computed goto.
     */
    #define BREAK break
    #define CHECK 
    #define CASE(opcode) case opcode
#endif

#if BLD_DEBUG
static EjsOpCode traceCode(Ejs *ejs, EjsOpCode opcode);
#define setFrameDebugName(frame, vp) (frame)->debugName = ((EjsVar*) (vp))->debugName
#else
static EjsOpCode traceCode(Ejs *ejs, EjsOpCode opcode);
// #define traceCode(ejs, opcode) opcode
#define setFrameDebugName(frame, name)
#endif

#if MPR_LITTLE_ENDIAN

#if 1
#define getByte(frame)              *(frame)->pc++

/*
 *  This is just temporary to avoid GCC warnings!
 */
static inline ushort getShort(EjsFrame *frame) {
    ushort  value, *sp = (ushort*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}

static inline uint getWord(EjsFrame *frame) {
    uint  value, *sp = (uint*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}

static inline uint64 getLong(EjsFrame *frame) {
    uint64  value, *sp = (uint64*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}

#if BLD_FEATURE_FLOATING_POINT
static inline double getDoubleWord(EjsFrame *frame) {
    double  value, *sp = (double*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}
#endif

#else

#define getByte(frame)              *(frame)->pc++
#define getShort(frame)             *((ushort*) ((frame)->pc))++
#define getWord(frame)              *((uint*) (frame)->pc)++
#define getLong(frame)              *((uint64*) (frame)->pc)++
#if BLD_FEATURE_FLOATING_POINT
#define getDoubleWord(frame)        *((double*) (frame)->pc)++
#endif

#endif

#else
//  TODO - add swapping code. NOTE: need to have in module what is the endian-ness of the module.
#endif


static EjsFrame *callBySlot(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *thisObj, int argc, int stackAdjust);
static inline EjsFrame *createFrame(Ejs *ejs);
static EjsFrame *callFunction(Ejs *ejs, EjsFunction *fun, EjsVar *thisObj, int argc, int stackAdjust);
static void callExceptionHandler(Ejs *ejs, EjsFunction *fun, int index, int flags);
static void debug(EjsFrame *frame);
static EjsVar *evalBinaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode, EjsVar *rhs);
static EjsVar *evalUnaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode);
static EjsName getNameArg(EjsFrame *frame);
static EjsVar *getNthBase(Ejs *ejs, EjsVar *obj, int nthBase);
static EjsVar *getNthBaseFromBottom(Ejs *ejs, EjsVar *obj, int nthBase);
static inline EjsVar *getNthBlock(EjsFrame *frame, int nth);
static int getNum(EjsFrame *frame);
static char *getStringArg(EjsFrame *frame);
static EjsVar *getGlobalArg(EjsFrame *frame);
static bool handleException(Ejs *ejs);
static void handleExceptionAtThisLevel(Ejs *ejs, EjsFrame *frame);
static void makeClosure(EjsFrame *frame);
static EjsFrame *getFunction(Ejs *ejs, EjsVar *thisObj, EjsVar *owner, int slotNum, EjsFunction *fun, EjsObject **local);
static void needClosure(EjsFrame *frame, EjsBlock *block);
static EjsFrame *payAttention(Ejs *ejs);
static EjsFrame *popExceptionFrame(Ejs *ejs);
static bool popFrameAndReturn(Ejs *ejs);
static void putFunction(Ejs *ejs, EjsVar *thisObj, EjsFunction *fun, EjsVar *value);
static void storeProperty(Ejs *ejs, EjsVar *obj, EjsName *name);
static void storePropertyToScope(Ejs *ejs, EjsName *qname);
static void swap2(Ejs *ejs);
static void throwNull(Ejs *ejs);
static EjsBlock *varToBlock(Ejs *ejs, EjsVar *vp);
static void vmLoop(Ejs *ejs);

/*
 *  Virtual Machine byte code evaluation
 */
static void vmLoop(Ejs *ejs)
{
    EjsFrame        *frame;
    EjsFunction     *fun;
    EjsString       *nameVar;
    EjsType         *type;
    EjsVar          *v1, *v2, *result, *vp, *vobj;
    EjsObject       *global, *local, *thisObj, *obj;
    EjsVar          **stack;
    EjsName         qname;
    EjsLookup       lookup;
    char            *str;
    uchar           *pc;
    int             i, argc, offset, slotNum, count, opcode, nthBase;

#if LINUX || MACOSX || LINUX || SOLARIS || VXWORKS 
    /*
     *  Direct threading computed goto processing. Include computed goto jump table.
     */
    static void *opcodeJump[] = {
        &&EJS_OP_ADD,
        &&EJS_OP_ADD_NAMESPACE,
        &&EJS_OP_ADD_NAMESPACE_REF,
        &&EJS_OP_AND,
        &&EJS_OP_BRANCH_EQ,
        &&EJS_OP_BRANCH_STRICTLY_EQ,
        &&EJS_OP_BRANCH_FALSE,
        &&EJS_OP_BRANCH_GE,
        &&EJS_OP_BRANCH_GT,
        &&EJS_OP_BRANCH_LE,
        &&EJS_OP_BRANCH_LT,
        &&EJS_OP_BRANCH_NE,
        &&EJS_OP_BRANCH_STRICTLY_NE,
        &&EJS_OP_BRANCH_NULL,
        &&EJS_OP_BRANCH_NOT_ZERO,
        &&EJS_OP_BRANCH_TRUE,
        &&EJS_OP_BRANCH_UNDEFINED,
        &&EJS_OP_BRANCH_ZERO,
        &&EJS_OP_BRANCH_FALSE_8,
        &&EJS_OP_BRANCH_TRUE_8,
        &&EJS_OP_BREAKPOINT,

        &&EJS_OP_CALL,
        &&EJS_OP_CALL_GLOBAL_SLOT,
        &&EJS_OP_CALL_OBJ_SLOT,
        &&EJS_OP_CALL_THIS_SLOT,
        &&EJS_OP_CALL_BLOCK_SLOT,
        &&EJS_OP_CALL_OBJ_INSTANCE_SLOT,
        &&EJS_OP_CALL_OBJ_STATIC_SLOT,
        &&EJS_OP_CALL_THIS_STATIC_SLOT,
        &&EJS_OP_CALL_OBJ_NAME,
        &&EJS_OP_CALL_SCOPED_NAME,
        &&EJS_OP_CALL_CONSTRUCTOR,
        &&EJS_OP_CALL_NEXT_CONSTRUCTOR,

        &&EJS_OP_CAST,
        &&EJS_OP_CAST_BOOLEAN,
        &&EJS_OP_CLOSE_BLOCK,
        &&EJS_OP_CLOSE_WITH,
        &&EJS_OP_COMPARE_EQ,
        &&EJS_OP_COMPARE_STRICTLY_EQ,
        &&EJS_OP_COMPARE_FALSE,
        &&EJS_OP_COMPARE_GE,
        &&EJS_OP_COMPARE_GT,
        &&EJS_OP_COMPARE_LE,
        &&EJS_OP_COMPARE_LT,
        &&EJS_OP_COMPARE_NE,
        &&EJS_OP_COMPARE_STRICTLY_NE,
        &&EJS_OP_COMPARE_NULL,
        &&EJS_OP_COMPARE_NOT_ZERO,
        &&EJS_OP_COMPARE_TRUE,
        &&EJS_OP_COMPARE_UNDEFINED,
        &&EJS_OP_COMPARE_ZERO,
        &&EJS_OP_DEBUG,
        &&EJS_OP_DEFINE_CLASS,
        &&EJS_OP_DEFINE_FUNCTION,
        &&EJS_OP_DEFINE_GLOBAL_FUNCTION,
        &&EJS_OP_DELETE_NAME_EXPR,
        &&EJS_OP_DELETE,
        &&EJS_OP_DELETE_NAME,
        &&EJS_OP_DIV,
        &&EJS_OP_DUP,
        &&EJS_OP_DUP2,
        &&EJS_OP_END_CODE,
        &&EJS_OP_END_EXCEPTION,
        &&EJS_OP_GOTO,
        &&EJS_OP_GOTO_8,
        &&EJS_OP_INC,
        &&EJS_OP_INIT_DEFAULT_ARGS,
        &&EJS_OP_INIT_DEFAULT_ARGS_8,
        &&EJS_OP_INST_OF,
        &&EJS_OP_IS_A,
        &&EJS_OP_LOAD_0,
        &&EJS_OP_LOAD_1,
        &&EJS_OP_LOAD_2,
        &&EJS_OP_LOAD_3,
        &&EJS_OP_LOAD_4,
        &&EJS_OP_LOAD_5,
        &&EJS_OP_LOAD_6,
        &&EJS_OP_LOAD_7,
        &&EJS_OP_LOAD_8,
        &&EJS_OP_LOAD_9,
        &&EJS_OP_LOAD_DOUBLE,
        &&EJS_OP_LOAD_FALSE,
        &&EJS_OP_LOAD_GLOBAL,
        &&EJS_OP_LOAD_INT_16,
        &&EJS_OP_LOAD_INT_32,
        &&EJS_OP_LOAD_INT_64,
        &&EJS_OP_LOAD_INT_8,
        &&EJS_OP_LOAD_M1,
        &&EJS_OP_LOAD_NAME,
        &&EJS_OP_LOAD_NAMESPACE,
        &&EJS_OP_LOAD_NULL,
        &&EJS_OP_LOAD_REGEXP,
        &&EJS_OP_LOAD_STRING,
        &&EJS_OP_LOAD_THIS,
        &&EJS_OP_LOAD_TRUE,
        &&EJS_OP_LOAD_UNDEFINED,
        &&EJS_OP_LOAD_XML,
        &&EJS_OP_GET_LOCAL_SLOT_0,
        &&EJS_OP_GET_LOCAL_SLOT_1,
        &&EJS_OP_GET_LOCAL_SLOT_2,
        &&EJS_OP_GET_LOCAL_SLOT_3,
        &&EJS_OP_GET_LOCAL_SLOT_4,
        &&EJS_OP_GET_LOCAL_SLOT_5,
        &&EJS_OP_GET_LOCAL_SLOT_6,
        &&EJS_OP_GET_LOCAL_SLOT_7,
        &&EJS_OP_GET_LOCAL_SLOT_8,
        &&EJS_OP_GET_LOCAL_SLOT_9,
        &&EJS_OP_GET_OBJ_SLOT_0,
        &&EJS_OP_GET_OBJ_SLOT_1,
        &&EJS_OP_GET_OBJ_SLOT_2,
        &&EJS_OP_GET_OBJ_SLOT_3,
        &&EJS_OP_GET_OBJ_SLOT_4,
        &&EJS_OP_GET_OBJ_SLOT_5,
        &&EJS_OP_GET_OBJ_SLOT_6,
        &&EJS_OP_GET_OBJ_SLOT_7,
        &&EJS_OP_GET_OBJ_SLOT_8,
        &&EJS_OP_GET_OBJ_SLOT_9,
        &&EJS_OP_GET_THIS_SLOT_0,
        &&EJS_OP_GET_THIS_SLOT_1,
        &&EJS_OP_GET_THIS_SLOT_2,
        &&EJS_OP_GET_THIS_SLOT_3,
        &&EJS_OP_GET_THIS_SLOT_4,
        &&EJS_OP_GET_THIS_SLOT_5,
        &&EJS_OP_GET_THIS_SLOT_6,
        &&EJS_OP_GET_THIS_SLOT_7,
        &&EJS_OP_GET_THIS_SLOT_8,
        &&EJS_OP_GET_THIS_SLOT_9,
        &&EJS_OP_GET_SCOPED_NAME,
        &&EJS_OP_GET_OBJ_NAME,
        &&EJS_OP_GET_OBJ_NAME_EXPR,
        &&EJS_OP_GET_BLOCK_SLOT,
        &&EJS_OP_GET_GLOBAL_SLOT,
        &&EJS_OP_GET_LOCAL_SLOT,
        &&EJS_OP_GET_OBJ_SLOT,
        &&EJS_OP_GET_THIS_SLOT,
        &&EJS_OP_GET_TYPE_SLOT,
        &&EJS_OP_GET_THIS_TYPE_SLOT,
        &&EJS_OP_IN,
        &&EJS_OP_LIKE,
        &&EJS_OP_LOGICAL_NOT,
        &&EJS_OP_MUL,
        &&EJS_OP_NEG,
        &&EJS_OP_NEW,
        &&EJS_OP_NEW_ARRAY,
        &&EJS_OP_NEW_OBJECT,
        &&EJS_OP_NOP,
        &&EJS_OP_NOT,
        &&EJS_OP_OPEN_BLOCK,
        &&EJS_OP_OPEN_WITH,
        &&EJS_OP_OR,
        &&EJS_OP_POP,
        &&EJS_OP_POP_ITEMS,
        &&EJS_OP_PUSH_CATCH_ARG,
        &&EJS_OP_PUSH_RESULT,
        &&EJS_OP_PUT_LOCAL_SLOT_0,
        &&EJS_OP_PUT_LOCAL_SLOT_1,
        &&EJS_OP_PUT_LOCAL_SLOT_2,
        &&EJS_OP_PUT_LOCAL_SLOT_3,
        &&EJS_OP_PUT_LOCAL_SLOT_4,
        &&EJS_OP_PUT_LOCAL_SLOT_5,
        &&EJS_OP_PUT_LOCAL_SLOT_6,
        &&EJS_OP_PUT_LOCAL_SLOT_7,
        &&EJS_OP_PUT_LOCAL_SLOT_8,
        &&EJS_OP_PUT_LOCAL_SLOT_9,
        &&EJS_OP_PUT_OBJ_SLOT_0,
        &&EJS_OP_PUT_OBJ_SLOT_1,
        &&EJS_OP_PUT_OBJ_SLOT_2,
        &&EJS_OP_PUT_OBJ_SLOT_3,
        &&EJS_OP_PUT_OBJ_SLOT_4,
        &&EJS_OP_PUT_OBJ_SLOT_5,
        &&EJS_OP_PUT_OBJ_SLOT_6,
        &&EJS_OP_PUT_OBJ_SLOT_7,
        &&EJS_OP_PUT_OBJ_SLOT_8,
        &&EJS_OP_PUT_OBJ_SLOT_9,
        &&EJS_OP_PUT_THIS_SLOT_0,
        &&EJS_OP_PUT_THIS_SLOT_1,
        &&EJS_OP_PUT_THIS_SLOT_2,
        &&EJS_OP_PUT_THIS_SLOT_3,
        &&EJS_OP_PUT_THIS_SLOT_4,
        &&EJS_OP_PUT_THIS_SLOT_5,
        &&EJS_OP_PUT_THIS_SLOT_6,
        &&EJS_OP_PUT_THIS_SLOT_7,
        &&EJS_OP_PUT_THIS_SLOT_8,
        &&EJS_OP_PUT_THIS_SLOT_9,
        &&EJS_OP_PUT_OBJ_NAME_EXPR,
        &&EJS_OP_PUT_SCOPED_NAME_EXPR,
        &&EJS_OP_PUT_OBJ_NAME,
        &&EJS_OP_PUT_SCOPED_NAME,
        &&EJS_OP_PUT_BLOCK_SLOT,
        &&EJS_OP_PUT_GLOBAL_SLOT,
        &&EJS_OP_PUT_LOCAL_SLOT,
        &&EJS_OP_PUT_OBJ_SLOT,
        &&EJS_OP_PUT_THIS_SLOT,
        &&EJS_OP_PUT_TYPE_SLOT,
        &&EJS_OP_PUT_THIS_TYPE_SLOT,
        &&EJS_OP_REM,
        &&EJS_OP_RETURN,
        &&EJS_OP_RETURN_VALUE,
        &&EJS_OP_SAVE_RESULT,
        &&EJS_OP_SHL,
        &&EJS_OP_SHR,
        &&EJS_OP_SUB,
        &&EJS_OP_SUPER,
        &&EJS_OP_SWAP,
        &&EJS_OP_THROW,
        &&EJS_OP_TYPE_OF,
        &&EJS_OP_USHR,
        &&EJS_OP_XOR,
    };
#endif

    mprAssert(ejs);
    mprAssert(!mprHasAllocError(ejs));

    frame = ejs->frame;
    stack = ejs->stack.top;
    global = (EjsObject*) ejs->global;
    local = (EjsObject*) frame->currentFunction;
    thisObj = global;
    pc = frame->pc;

    /*
     *  Just to keep the compiler happy
     */
    slotNum = -1;
    vp = 0;

#if LINUX || MACOSX || LINUX || SOLARIS || VXWORKS 
    /*
     *  Direct threading computed goto processing. Include computed goto jump table.
     */
    BREAK;

#else
    /*
     *  Traditional switch for compilers (looking at you MS) without computed goto.
     */
    while (1) {
        opcode = (EjsOpCode) getByte(frame);
        traceCode(ejs, opcode);
        switch (opcode) {
#endif
        /*
         *  Symbolic source code debug information
         *      Debug <fileName> <lineNumber> <sourceLine>
         */
        CASE (EJS_OP_DEBUG):
            debug(frame);
            BREAK;

        /*
         *  End of a code block. Used to mark the end of a script. Saves testing end of code block in VM loop.
         *      EndCode
         */
        CASE (EJS_OP_END_CODE):
            /*
             *  The "ejs" command needs to preserve the current ejs->result for interactive sessions.
             */
            if (ejs->result == 0) {
                ejs->result = frame->returnValue = ejs->undefinedValue;
            }
            popFrameAndReturn(ejs);
            return;

        /*
         *  Return from a function with a result
         *      ReturnValue
         *      Stack before (top)  [value]
         *      Stack after         []
         *      Frame return value set
         */
        CASE (EJS_OP_RETURN_VALUE):
            frame->returnValue = pop(ejs);
            ejs->result = frame->returnValue;
            if (popFrameAndReturn(ejs)) {
                return;
            }
            frame = ejs->frame;
            local = (EjsObject*) frame->currentFunction;
            CHECK; BREAK;

        /*
         *  Return from a function without a result
         *      Return
         */
        CASE (EJS_OP_RETURN):
            frame->returnValue = 0;
            ejs->result = ejs->undefinedValue;
            if (popFrameAndReturn(ejs)) {
                return;
            }
            frame = ejs->frame;
            local = (EjsObject*) frame->currentFunction;
            CHECK; BREAK;

        /*
         *  Load the catch argument
         *      PushCatchArg
         *      Stack before (top)  []
         *      Stack after         [catchArg]
         */
        CASE (EJS_OP_PUSH_CATCH_ARG):
            /*
             *  The exception argument has been pushed on the stack before the catch block
             */
            push(ejs, frame->exceptionArg);
            BREAK;

        /*
         *  Push the function call result
         *      PushResult
         *      Stack before (top)  []
         *      Stack after         [result]
         */
        CASE (EJS_OP_PUSH_RESULT):
            push(ejs, frame->returnValue);
            BREAK;

        /*
         *  Save the top of stack and store in the interpreter result register
         *      SaveResult
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_SAVE_RESULT):
            ejs->result = frame->returnValue = pop(ejs);
            BREAK;


        /*
         *  Load Constants -----------------------------------------------
         */

        /*
         *  Load a signed 8 bit integer constant
         *      LoadInt.8           <int8>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_8):
            push(ejs, ejsCreateNumber(ejs, getByte(frame)));
            CHECK; BREAK;

        /*
         *  Load a signed 16 bit integer constant
         *      LoadInt.16          <int16>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_16):
            push(ejs, ejsCreateNumber(ejs, getShort(frame)));
            CHECK; BREAK;

        /*
         *  Load a signed 32 bit integer constant
         *      LoadInt.32          <int32>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_32):
            push(ejs, ejsCreateNumber(ejs, getWord(frame)));
            CHECK; BREAK;

        /*
         *  Load a signed 64 bit integer constant
         *      LoadInt.64          <int64>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_64):
        push(ejs, ejsCreateNumber(ejs, (MprNumber) getLong(frame)));
        CHECK; BREAK;

        /*
         *  Load a float constant
         *      LoadDouble          <double>
         *      Stack before (top)  []
         *      Stack after         [Double]
         */
        CASE (EJS_OP_LOAD_DOUBLE):
#if BLD_FEATURE_FLOATING_POINT
            push(ejs, ejsCreateNumber(ejs, getDoubleWord(frame)));
#endif
            CHECK; BREAK;

        /*
         *  Load integer constant between 0 and 9
         *      Load0, Load1, ... Load9
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_0):
        CASE (EJS_OP_LOAD_1):
        CASE (EJS_OP_LOAD_2):
        CASE (EJS_OP_LOAD_3):
        CASE (EJS_OP_LOAD_4):
        CASE (EJS_OP_LOAD_5):
        CASE (EJS_OP_LOAD_6):
        CASE (EJS_OP_LOAD_7):
        CASE (EJS_OP_LOAD_8):
        CASE (EJS_OP_LOAD_9):
            push(ejs, ejsCreateNumber(ejs, opcode - EJS_OP_LOAD_0));
            CHECK; BREAK;

        /*
         *  Load the -1 integer constant
         *      LoadMinusOne
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_M1):
            push(ejs, ejsCreateNumber(ejs, -1));
            BREAK;

        /*
         *  Load a string constant (TODO - Unicode)
         *      LoadString          <string>
         *      Stack before (top)  []
         *      Stack after         [String]
         */
        CASE (EJS_OP_LOAD_STRING):
            str = getStringArg(frame);
            push(ejs, ejsCreateString(ejs, str));
            CHECK; BREAK;

        /*
         *  Load a namespace constant
         *      LoadNamespace       <UriString>
         *      Stack before (top)  []
         *      Stack after         [Namespace]
         */
        CASE (EJS_OP_LOAD_NAMESPACE):
            /*
             *  TODO - This is the namespace URI. missing the name itself.  TODO Unicode.
             */
            str = getStringArg(frame);
            push(ejs, ejsCreateNamespace(ejs, str, str));
            CHECK; BREAK;


        /*
         *  Load an XML constant
         *      LoadXML             <xmlString>
         *      Stack before (top)  []
         *      Stack after         [XML]
         */
        CASE (EJS_OP_LOAD_XML):
#if BLD_FEATURE_EJS_E4X
            v1 = (EjsVar*) ejsCreateXML(ejs, 0, 0, 0, 0);
            str = getStringArg(frame);
            ejsLoadXMLString(ejs, (EjsXML*) v1, str);
            push(ejs, v1);
#endif
            CHECK; BREAK;

        /*
         *  Load a Regexp constant
         *      LoadRegExp
         *      Stack before (top)  []
         *      Stack after         [RegExp]
         */
        CASE (EJS_OP_LOAD_REGEXP):
            str = getStringArg(frame);
#if BLD_FEATURE_REGEXP
            v1 = (EjsVar*) ejsCreateRegExp(ejs, str);
            push(ejs, v1);
#else
            ejsThrowReferenceError(ejs, "No regular expression support");
#endif
            CHECK; BREAK;

        /*
         *  Load a null constant
         *      LoadNull
         *      Stack before (top)  []
         *      Stack after         [Null]
         */
        CASE (EJS_OP_LOAD_NULL):
            push(ejs, ejs->nullValue);
            BREAK;

        /*
         *  Load a void / undefined constant
         *      LoadUndefined
         *      Stack before (top)  []
         *      Stack after         [undefined]
         */
        CASE (EJS_OP_LOAD_UNDEFINED):                           //  TODO - rename LOAD_UNDEFINED
            push(ejs, ejs->undefinedValue);
            BREAK;

        /*
         *  Load the "this" value
         *      LoadThis
         *      Stack before (top)  []
         *      Stack after         [this]
         */
        CASE (EJS_OP_LOAD_THIS):
            mprAssert(frame->thisObj);
            push(ejs, frame->thisObj);
            BREAK;

        /*
         *  Load the "global" value
         *      LoadGlobal
         *      Stack before (top)  []
         *      Stack after         [global]
         */
        CASE (EJS_OP_LOAD_GLOBAL):
            push(ejs, ejs->global);
            BREAK;

        /*
         *  Load the "true" value
         *      LoadTrue
         *      Stack before (top)  []
         *      Stack after         [true]
         */
        CASE (EJS_OP_LOAD_TRUE):
            push(ejs, ejs->trueValue);
            BREAK;

        /*
         *  Load the "false" value
         *      LoadFalse
         *      Stack before (top)  []
         *      Stack after         [false]
         */
        CASE (EJS_OP_LOAD_FALSE):
            push(ejs, ejs->falseValue);
            BREAK;


        /*
         *  Push a global variable by slot number
         *      GetGlobalSlot       <slot>
         *      Stack before (top)  []
         *      Stack after         [PropRef]
         */
        CASE (EJS_OP_GET_GLOBAL_SLOT):
            slotNum = getByte(frame);
            GET_PROPERTY(ejs, NULL, global, slotNum);
            CHECK; BREAK;

        /*
         *  Push a local variable by slot number
         *      GetLocalSlot        <slot>
         *      Stack before (top)  []
         *      Stack after         [PropRef]
         */
        CASE (EJS_OP_GET_LOCAL_SLOT):
            slotNum = getByte(frame);
            GET_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Push a local variable in slot 0-9
         *      GetLocalSlot0, GetLocalSlot1, ... GetLocalSlot9
         *      Stack before (top)  []
         *      Stack after         [PropRef]
         */
        CASE (EJS_OP_GET_LOCAL_SLOT_0):
        CASE (EJS_OP_GET_LOCAL_SLOT_1):
        CASE (EJS_OP_GET_LOCAL_SLOT_2):
        CASE (EJS_OP_GET_LOCAL_SLOT_3):
        CASE (EJS_OP_GET_LOCAL_SLOT_4):
        CASE (EJS_OP_GET_LOCAL_SLOT_5):
        CASE (EJS_OP_GET_LOCAL_SLOT_6):
        CASE (EJS_OP_GET_LOCAL_SLOT_7):
        CASE (EJS_OP_GET_LOCAL_SLOT_8):
        CASE (EJS_OP_GET_LOCAL_SLOT_9):
            slotNum = opcode - EJS_OP_GET_LOCAL_SLOT_0;
            GET_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Push a block scoped variable by slot number
         *      GetBlockSlot        <slot> <nthBlock>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_BLOCK_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) getNthBlock(frame, getByte(frame));
            GET_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in thisObj by slot number
         *      GetThisSlot         <slot>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_THIS_SLOT):
            slotNum = getByte(frame);
            GET_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in slot 0-9
         *      GetThisSlot0, GetThisSlot1,  ... GetThisSlot9
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_THIS_SLOT_0):
        CASE (EJS_OP_GET_THIS_SLOT_1):
        CASE (EJS_OP_GET_THIS_SLOT_2):
        CASE (EJS_OP_GET_THIS_SLOT_3):
        CASE (EJS_OP_GET_THIS_SLOT_4):
        CASE (EJS_OP_GET_THIS_SLOT_5):
        CASE (EJS_OP_GET_THIS_SLOT_6):
        CASE (EJS_OP_GET_THIS_SLOT_7):
        CASE (EJS_OP_GET_THIS_SLOT_8):
        CASE (EJS_OP_GET_THIS_SLOT_9):
            slotNum = opcode - EJS_OP_GET_THIS_SLOT_0;
            GET_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in an object by slot number
         *      GetObjSlot          <slot>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_OBJ_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) pop(ejs);
            GET_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in an object from slot 0-9
         *      GetObjSlot0, GetObjSlot1, ... GetObjSlot9
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_OBJ_SLOT_0):
        CASE (EJS_OP_GET_OBJ_SLOT_1):
        CASE (EJS_OP_GET_OBJ_SLOT_2):
        CASE (EJS_OP_GET_OBJ_SLOT_3):
        CASE (EJS_OP_GET_OBJ_SLOT_4):
        CASE (EJS_OP_GET_OBJ_SLOT_5):
        CASE (EJS_OP_GET_OBJ_SLOT_6):
        CASE (EJS_OP_GET_OBJ_SLOT_7):
        CASE (EJS_OP_GET_OBJ_SLOT_8):
        CASE (EJS_OP_GET_OBJ_SLOT_9):
            vp = pop(ejs);
            slotNum = opcode - EJS_OP_GET_OBJ_SLOT_0;
            GET_PROPERTY(ejs, NULL, vp, slotNum);
            CHECK; BREAK;


        /*
         *  Push a variable from a type by slot number
         *      GetTypeSlot         <slot> <nthBase>
         *      Stack before (top)  [objRef]
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_TYPE_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) pop(ejs);
            vp = getNthBase(ejs, (EjsVar*) obj, getNum(frame));
            GET_PROPERTY(ejs, (EjsVar*) obj, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Push a type variable by slot number from this. NthBase counts from Object up rather than "this" down.
         *      GetThisTypeSlot     <slot> <nthBaseFromBottom>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_THIS_TYPE_SLOT):
            slotNum = getNum(frame);
            type = (EjsType*) getNthBaseFromBottom(ejs, frame->thisObj, getNum(frame));
            if (type == 0) {
                ejsThrowReferenceError(ejs, "Bad base class reference");
            } else {
                GET_PROPERTY(ejs, frame->thisObj, type, slotNum);
            }
            CHECK; BREAK;

        /*
         *  Push a variable by an unqualified name
         *      GetScopedName       <qname>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_SCOPED_NAME):
            qname = getNameArg(frame);
            result = ejsGetVarByName(ejs, 0, &qname, 1, &lookup);
            if (result) {
                if (ejsIsFunction(result)) {
                    GET_PROPERTY(ejs, NULL, lookup.obj, lookup.slotNum);
                } else {
                    push(ejs, result);
                }
            } else {
                push(ejs, ejs->nullValue);
            }
            CHECK; BREAK;
                
        /*
         *  Get a property by property name
         *      GetObjName          <qname>
         *      Stack before (top)  [obj]
         *      Stack after         [result]
         */
        CASE (EJS_OP_GET_OBJ_NAME):
            qname = getNameArg(frame);
            vp = pop(ejs);
            result = ejsGetVarByName(ejs, vp, &qname, 1, &lookup);
            if (result) {
                if (ejsIsFunction(result)) {
                    GET_PROPERTY(ejs, vp, lookup.obj, lookup.slotNum);
                } else {
                    push(ejs, result);
                }
            } else {
                push(ejs, ejs->nullValue);
            }
            CHECK; BREAK;

        /*
         *  Get a property by property name expression
         *      GetObjNameExpr
         *      Stack before (top)  [propName]
         *                          [obj]
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_OBJ_NAME_EXPR):
            v1 = pop(ejs);
            vp = pop(ejs);
            if (vp->type->numericIndicies && ejsIsNumber(v1)) {
                push(ejs, ejsGetProperty(ejs, vp, ejsGetInt(v1)));
                CHECK; BREAK;
            }
            nameVar = ejsToString(ejs, v1);
            if (nameVar) {
                qname.name = nameVar->value;
                qname.space = "";
                result = ejsGetVarByName(ejs, vp, &qname, 1, &lookup);
                if (result) {
                    if (ejsIsFunction(result)) {
                        GET_PROPERTY(ejs, vp, lookup.obj, lookup.slotNum);
                    } else {
                        push(ejs, result);
                    }
                } else {
                    push(ejs, ejs->nullValue);
                }
            }
            CHECK; BREAK;

        /*
         *  Store -------------------------------
         */

        /*
         *  Pop a global variable by slot number
         *      Stack before (top)  [value]
         *      Stack after         []
         *      PutGlobalSlot       <slot>
         */
        CASE (EJS_OP_PUT_GLOBAL_SLOT):
            slotNum = getByte(frame);
            mprAssert(slotNum < 256);
            PUT_PROPERTY(ejs, NULL, global, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a local variable by slot number
         *      Stack before (top)  [value]
         *      Stack after         []
         *      PutLocalSlot        <slot>
         */
        CASE (EJS_OP_PUT_LOCAL_SLOT):
            slotNum = getByte(frame);
            PUT_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a local variable from slot 0-9
         *      PutLocalSlot0, PutLocalSlot1, ... PutLocalSlot9
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_LOCAL_SLOT_0):
        CASE (EJS_OP_PUT_LOCAL_SLOT_1):
        CASE (EJS_OP_PUT_LOCAL_SLOT_2):
        CASE (EJS_OP_PUT_LOCAL_SLOT_3):
        CASE (EJS_OP_PUT_LOCAL_SLOT_4):
        CASE (EJS_OP_PUT_LOCAL_SLOT_5):
        CASE (EJS_OP_PUT_LOCAL_SLOT_6):
        CASE (EJS_OP_PUT_LOCAL_SLOT_7):
        CASE (EJS_OP_PUT_LOCAL_SLOT_8):
        CASE (EJS_OP_PUT_LOCAL_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_LOCAL_SLOT_0;
            PUT_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a block variable by slot number
         *      PutBlockSlot        <slot> <nthBlock>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_BLOCK_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) getNthBlock(frame, getNum(frame));
            PUT_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;

#if FUTURE
        /*
         *  Pop a block variable from slot 0-9
         *      PutBlockSlot0, PutBlockSlot1, ... PutBlockSlot9 <nthBlock>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_BLOCK_SLOT_0):
        CASE (EJS_OP_PUT_BLOCK_SLOT_1):
        CASE (EJS_OP_PUT_BLOCK_SLOT_2):
        CASE (EJS_OP_PUT_BLOCK_SLOT_3):
        CASE (EJS_OP_PUT_BLOCK_SLOT_4):
        CASE (EJS_OP_PUT_BLOCK_SLOT_5):
        CASE (EJS_OP_PUT_BLOCK_SLOT_6):
        CASE (EJS_OP_PUT_BLOCK_SLOT_7):
        CASE (EJS_OP_PUT_BLOCK_SLOT_8):
        CASE (EJS_OP_PUT_BLOCK_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_BLOCK_SLOT_0;
            obj = (EjsObject*) getNthBlock(frame, getNum(frame));
            PUT_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;
#endif

        /*
         *  Store a property by slot number
         *      PutThisSlot         <slot>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_THIS_SLOT):
            slotNum = getByte(frame);
            PUT_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Store a property to slot 0-9
         *      PutThisSlot0, PutThisSlot1, ... PutThisSlot9,
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_THIS_SLOT_0):
        CASE (EJS_OP_PUT_THIS_SLOT_1):
        CASE (EJS_OP_PUT_THIS_SLOT_2):
        CASE (EJS_OP_PUT_THIS_SLOT_3):
        CASE (EJS_OP_PUT_THIS_SLOT_4):
        CASE (EJS_OP_PUT_THIS_SLOT_5):
        CASE (EJS_OP_PUT_THIS_SLOT_6):
        CASE (EJS_OP_PUT_THIS_SLOT_7):
        CASE (EJS_OP_PUT_THIS_SLOT_8):
        CASE (EJS_OP_PUT_THIS_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_THIS_SLOT_0;
            PUT_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Store a property by slot number
         *      PutObjSlot          <slot>
         *      Stack before (top)  [obj]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_SLOT):
            slotNum = getByte(frame);
            vp = pop(ejs);
            PUT_PROPERTY(ejs, NULL, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Store a property to slot 0-9
         *      PutObjSlot0, PutObjSlot1, ... PutObjSlot9
         *      Stack before (top)  [obj]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_SLOT_0):
        CASE (EJS_OP_PUT_OBJ_SLOT_1):
        CASE (EJS_OP_PUT_OBJ_SLOT_2):
        CASE (EJS_OP_PUT_OBJ_SLOT_3):
        CASE (EJS_OP_PUT_OBJ_SLOT_4):
        CASE (EJS_OP_PUT_OBJ_SLOT_5):
        CASE (EJS_OP_PUT_OBJ_SLOT_6):
        CASE (EJS_OP_PUT_OBJ_SLOT_7):
        CASE (EJS_OP_PUT_OBJ_SLOT_8):
        CASE (EJS_OP_PUT_OBJ_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_OBJ_SLOT_0;
            vp = pop(ejs);
            PUT_PROPERTY(ejs, NULL, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a variable by an unqualified name
         *      PutScopedName       <qname>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_SCOPED_NAME):
            qname = getNameArg(frame);
            storePropertyToScope(ejs, &qname);
            CHECK; BREAK;

        /*
         *  Pop a variable by an unqualified property name expr
         *      PutNameExpr
         *      Stack before (top)  [name]
         *                          [value]
         *      Stack after         []
         */
        //  TODO - this op code looks like its operands are swapped on the stack.
        CASE (EJS_OP_PUT_SCOPED_NAME_EXPR):
            swap2(ejs);
            nameVar = popString(ejs);
            nameVar = ejsToString(ejs, (EjsVar*) nameVar);
            if (nameVar) {
                //  TODO - BUG. Must strdup the name
                qname.name = nameVar->value;
                qname.space = 0;
                storePropertyToScope(ejs, &qname);
            }
            CHECK; BREAK;

        /*
         *  Pop a property by property name to an object
         *      PutObjName
         *      Stack before (top)  [value]
         *                          [objRef]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_NAME):
            qname = getNameArg(frame);
            storeProperty(ejs, pop(ejs), &qname);
            CHECK; BREAK;

        /*
         *  Pop a property by property name expression to an object
         *      PutObjNameExpr
         *      Stack before (top)  [nameExpr]
         *                          [objRef]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_NAME_EXPR):
            v1 = pop(ejs);
            vp = pop(ejs);
            if (vp->type->numericIndicies && ejsIsNumber(v1)) {
                ejsSetProperty(ejs, vp, ejsGetInt(v1), pop(ejs));
            } else {
                nameVar = ejsToString(ejs, v1);
                if (nameVar) {
                    //  TODO BUG - not freeing old property name if it was alloced.
                    qname.name = mprStrdup(vp, nameVar->value);
                    qname.space = EJS_PUBLIC_NAMESPACE;
                    storeProperty(ejs, vp, &qname);
                }
            }
            CHECK; BREAK;

        /*
         *  Pop a type variable by slot number
         *      PutTypeSlot         <slot> <nthBase>
         *      Stack before (top)  [obj]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_TYPE_SLOT):
            slotNum = getNum(frame);
            vobj = pop(ejs);
            vp = getNthBase(ejs, vobj, getNum(frame));
            PUT_PROPERTY(ejs, vobj, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a variable to a slot in the nthBase class of the current "this" object
         *      PutThisTypeSlot     <slot> <nthBase>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_THIS_TYPE_SLOT):
            //  TODO - CLEANUP
            // v1 = pop(ejs);
            slotNum = getNum(frame);
            type = (EjsType*) getNthBaseFromBottom(ejs, frame->thisObj, getNum(frame));
            if (type == 0) {
                ejsThrowReferenceError(ejs, "Bad base class reference");
            } else {
                PUT_PROPERTY(ejs, frame->thisObj, (EjsVar*) type, slotNum);
                // ejsSetProperty(ejs, (EjsVar*) type, slotNum, v1);
            }
            CHECK; BREAK;


        /*
         *  Function calling and return
         */

        /*
         *  Call a function by reference
         *      Stack before (top)  [args]
         *                          [function]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL):
            argc = getNum(frame);
            vp = ejs->stack.top[-argc - 1];
            fun = (EjsFunction*) ejs->stack.top[-argc];
            frame = callFunction(ejs, fun, vp, argc, 2);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a global function by slot on the given type
         *      CallGlobalSlot      <slot> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_GLOBAL_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            frame = callFunction(ejs, (EjsFunction*) global->slots[slotNum], ejs->global, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on the pushed object
         *      CallObjSlot         <slot> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
            } else {
                frame = callFunction(ejs, (EjsFunction*) vp->type->block.obj.slots[slotNum], vp, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on the current this object.
         *      CallThisSlot        <slot> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_THIS_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            if (ejsIsFunction(&frame->function)) {
                obj = (EjsObject*) frame->function.owner;
            } else {
                obj = (EjsObject*) frame->thisObj->type;
                mprAssert(obj != (EjsObject*) ejs->typeType);
            }
            mprAssert(ejsIsObject(obj));
            mprAssert(!ejsIsFunction(obj));
            frame = callFunction(ejs, (EjsFunction*) obj->slots[slotNum], frame->thisObj, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on the nth enclosing block
         *      CallBlockSlot        <slot> <nthBlock> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_BLOCK_SLOT):
            slotNum = getNum(frame);
            obj = (EjsObject*) getNthBlock(frame, getNum(frame));
            argc = getNum(frame);
            frame = callFunction(ejs, (EjsFunction*) obj->slots[slotNum], frame->thisObj, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on an object.
         *      CallObjInstanceSlot <slot> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_INSTANCE_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
            } else {
                frame = callBySlot(ejs, vp, slotNum, vp, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a static function by slot number on the pushed object
         *      CallObjStaticSlot   <slot> <nthBase> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_STATIC_SLOT):
            slotNum = getNum(frame);
            nthBase = getNum(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
            } else {
                type = (EjsType*) getNthBase(ejs, vp, nthBase);
                frame = callFunction(ejs, (EjsFunction*) type->block.obj.slots[slotNum], (EjsVar*) type, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a static function by slot number on the nth base class of the current "this" object
         *      CallThisStaticSlot  <slot> <nthBase> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_THIS_STATIC_SLOT):
            slotNum = getNum(frame);
            nthBase = getNum(frame);
            argc = getNum(frame);
            type = (EjsType*) getNthBase(ejs, frame->thisObj, nthBase);
            frame = callFunction(ejs, (EjsFunction*) type->block.obj.slots[slotNum], (EjsVar*) type, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by name on the pushed object
         *      CallObjName         <qname> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_NAME):
            qname = getNameArg(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            //  TODO - what about undefined?
            if (vp == 0 /* || vp == ejs->nullValue */) {
                throwNull(ejs);
                CHECK; BREAK;
            }
            slotNum = ejsLookupVar(ejs, (EjsVar*) vp, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Can't find function \"%s\"", qname.name);
            } else {
                frame = callBySlot(ejs, (EjsVar*) lookup.obj, slotNum, vp, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a function by name in the current scope. Use existing "this" object if defined.
         *      CallName            <qname> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_SCOPED_NAME):
            qname = getNameArg(frame);
            argc = getNum(frame);
            slotNum = ejsLookupScope(ejs, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Can't find method %s", qname.name);
                CHECK; BREAK;
            }
            /*
             *  Calculate the "this" to use for the function. If required function is a method in the current "this" object
             *  use the current thisObj. If the lookup.obj is a type, then use it. Otherwise global.
             */
            if (ejsIsA(ejs, frame->thisObj, (EjsType*) lookup.obj)) {
                v1 = frame->thisObj;
            } else if (ejsIsType(lookup.obj)) {
                v1 = lookup.obj;
            } else {
                v1 = ejs->global;
            }
            frame = callBySlot(ejs, lookup.obj, lookup.slotNum, v1, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a constructor
         *      CallConstructor     <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         [obj]
         */
        CASE (EJS_OP_CALL_CONSTRUCTOR):
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
                CHECK; BREAK;
            }
            type = vp->type;
            mprAssert(type);
            if (type && type->hasConstructor) {
                mprAssert(type->baseType);
                //  Constructor is always at slot 0 (offset by base properties)
                slotNum = type->block.numInherited;
                frame = callBySlot(ejs, (EjsVar*) type, slotNum, vp, argc, 0);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call the next constructor
         *      CallNextConstructor <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_NEXT_CONSTRUCTOR):
            argc = getNum(frame);
            fun = &frame->function;
            type = (EjsType*) fun->owner;
            mprAssert(type);
            type = type->baseType;
            if (type) {
                mprAssert(type->hasConstructor);
                slotNum = type->block.numInherited;
                vp = frame->thisObj;
                frame = callBySlot(ejs, (EjsVar*) type, slotNum, vp, argc, 0);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            } else {
                mprAssert(0);
            }
            CHECK; BREAK;

        /*
         *  Add namespace to the set of open namespaces for the current block
         *      AddNamespace <string>
         */
        CASE (EJS_OP_ADD_NAMESPACE):
            /*
             *  TODO this is the namespace URI, missing the name. TODO - unicode.
             */
            str = getStringArg(frame);
            ejsAddNamespaceToBlock(ejs, &frame->function.block, ejsCreateNamespace(ejs, str, str));
            CHECK; BREAK;

        /*
         *  Add namespace reference to the set of open namespaces for the current block. (use namespace).
         *      Stack before (top)  [namespace]
         *      Stack after         []
         *      AddNamespaceRef
         */
        CASE (EJS_OP_ADD_NAMESPACE_REF):
            ejsAddNamespaceToBlock(ejs, &frame->function.block, (EjsNamespace*) pop(ejs));
            CHECK; BREAK;

        /*
         *  Push a new scope block on the scope chain
         *      OpenBlock <slotNum> <nthBlock>
         */
        CASE (EJS_OP_OPEN_BLOCK):
            slotNum = getNum(frame);
            vp = getNthBlock(frame, getNum(frame));
            v1 = ejsGetProperty(ejs, vp, slotNum);
            if (v1 == 0 || !ejsIsBlock(v1)) {
                ejsThrowReferenceError(ejs, "Reference is not a block");
                CHECK; BREAK;
            }
            frame = ejsPushFrame(ejs, (EjsBlock*) v1);
            CHECK; BREAK;

        /*
         *  Add a new scope block from the stack onto on the scope chain
         *      OpenWith
         */
        CASE (EJS_OP_OPEN_WITH):
            frame = ejsPushFrame(ejs, varToBlock(ejs, pop(ejs)));
            CHECK; BREAK;

        /*
         *  Pop the top scope block off the scope chain
         *      CloseBlock
         *      CloseWith
         *  TODO - merge these
         */
        CASE (EJS_OP_CLOSE_BLOCK):
        CASE (EJS_OP_CLOSE_WITH):
            frame = ejsPopFrame(ejs);
            CHECK; BREAK;

        /*
         *  Define a class and initialize by calling any static initializer.
         *      DefineClass <type>
         */
        CASE (EJS_OP_DEFINE_CLASS):
            type = getType(frame);
            if (type == 0 || !ejsIsType(type)) {
                ejsThrowReferenceError(ejs, "Reference is not a class");
            } else {
                needClosure(frame, (EjsBlock*) type);
                if (type && type->hasStaticInitializer) {
                    mprAssert(type->baseType);
                    //  Initializer is always immediately after the constructor (if present)
                    slotNum = type->block.numInherited;
                    if (type->hasConstructor) {
                        slotNum++;
                    }
                    frame = callBySlot(ejs, (EjsVar*) type, slotNum, ejs->global, 0, 0);
                    if (frame) {
                        local = (EjsObject*) frame->currentFunction;
                    }
                }
            }
            ejs->result = (EjsVar*) type;
            CHECK; BREAK;

        /*
         *  Define a function. This is used for non-method functions.
         *      DefineFunction <slot> <nthBlock>
         */
        CASE (EJS_OP_DEFINE_FUNCTION):
            slotNum = getNum(frame);
            vp = getNthBlock(frame, getNum(frame));
            mprAssert(vp != ejs->global);
            fun = (EjsFunction*) ejsGetProperty(ejs, vp, slotNum);
            if (fun == 0 || !ejsIsFunction(fun)) {
                ejsThrowReferenceError(ejs, "Reference is not a function");
            } else {
                if (fun->fullScope) {
                    needClosure(frame, (EjsBlock*) fun);
                    fun->thisObj = frame->thisObj;
                }
            }
            CHECK; BREAK;

        /*
         *  Define a global function
         *      DefineGlobalFunction <Global>
         */
        CASE (EJS_OP_DEFINE_GLOBAL_FUNCTION):
            fun = (EjsFunction*) getGlobalArg(frame);
            if (fun == 0 || !ejsIsFunction(fun)) {
                ejsThrowReferenceError(ejs, "Global reference is not a function");
            } else {
                /* 
                 *  Add or 1 because function (a=a) is not binding RHS correctly
                 */
                if (fun->fullScope || 1) {
                    needClosure(frame, (EjsBlock*) fun);
                }
            }
            CHECK; BREAK;

        /*
         *  Exception Handling --------------------------------------------
         */

        /*
         *  End of an exception block
         *      EndException
         */
        CASE (EJS_OP_END_EXCEPTION):
            if (frame->inCatch || frame->inFinally) {
                frame = popExceptionFrame(ejs);
                frame->pc = frame->endException;
            }
            CHECK; BREAK;

        /*
         *  Throw an exception
         *      Stack before (top)  [exceptionObj]
         *      Stack after         []
         *      Throw
         */
        CASE (EJS_OP_THROW):
            ejs->exception = pop(ejs);
            ejs->attention = 1;
            CHECK; BREAK;

        /*
         *  Stack management ----------------------------------------------
         */

        /*
         *  Pop one item off the stack
         *      Pop
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_POP):
            ejs->result = pop(ejs);
            BREAK;

        /*
         *  Pop N items off the stack
         *      PopItems            <count>
         *      Stack before (top)  [value]
         *                          [...]
         *      Stack after         []
         */
        CASE (EJS_OP_POP_ITEMS):
            ejs->stack.top -= getByte(frame);
            ejs->result = ejs->stack.top[1];
            BREAK;

        /*
         *  Duplicate one item on the stack
         *      Stack before (top)  [value]
         *      Stack after         [value]
         *                          [value]
         */
        CASE (EJS_OP_DUP):
            v1 = ejs->stack.top[0];
            push(ejs, v1);
            BREAK;

        /*
         *  Duplicate two items on the stack
         *      Dup2
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [value1]
         *                          [value2]
         *                          [value1]
         *                          [value2]
         */
        CASE (EJS_OP_DUP2):
            v1 = ejs->stack.top[-1];
            push(ejs, v1);
            v1 = ejs->stack.top[0];
            push(ejs, v1);
            BREAK;

        /*
         *  Swap the top two items on the stack
         *      Swap
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [value2]
         *                          [value1]
         */
        CASE (EJS_OP_SWAP):
            swap2(ejs);
            BREAK;

        /*
         *  Branching
         */

        /*
         *  Default function argument initialization. Computed goto for functions with more than 256 parameters.
         *      InitDefault         <tableSize>
         */
        CASE (EJS_OP_INIT_DEFAULT_ARGS): {
            int tableSize, numNonDefault;
            /*
             *  Use the argc value for the current function. Compare with the number of default args.
             */
            tableSize = (char) getByte(frame);
            fun = &frame->function;
            numNonDefault = fun->numArgs - fun->numDefault;
            mprAssert(numNonDefault >= 0);
            offset = frame->argc - numNonDefault;
            if (offset < 0 || offset > tableSize) {
                offset = tableSize;
            }
            //  TODO - ENDIAN
            frame->pc += ((uint*) frame->pc)[offset];
            CHECK; BREAK;
        }

        /*
         *  Default function argument initialization. Computed goto for functions with less than 256 parameters.
         *      InitDefault.8       <tableSize.8>
         */
        CASE (EJS_OP_INIT_DEFAULT_ARGS_8): {
            int tableSize, numNonDefault;

            tableSize = (char) getByte(frame);
            fun = &frame->function;
            numNonDefault = fun->numArgs - fun->numDefault;
            mprAssert(numNonDefault >= 0);
            offset = frame->argc - numNonDefault;
            if (offset < 0 || offset > tableSize) {
                offset = tableSize;
            }
            frame->pc += frame->pc[offset];
            CHECK; BREAK;
        }

        /*
         *  Unconditional branch to a new location
         *      Goto                <offset.32>
         */
        CASE (EJS_OP_GOTO):
            offset = getWord(frame);
            frame->pc = &frame->pc[offset];
            BREAK;

        /*
         *  Unconditional branch to a new location (8 bit)
         *      Goto.8              <offset.8>
         */
        CASE (EJS_OP_GOTO_8):
            offset = (char) getByte(frame);
            frame->pc = &frame->pc[offset];
            BREAK;

        /*
         *  Branch to offset if false
         *      BranchFalse
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_FALSE):
            offset = getWord(frame);
            goto commonBoolBranchCode;

        /*
         *  Branch to offset if true
         *      BranchTrue
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_TRUE):
            offset = getWord(frame);
            goto commonBoolBranchCode;

        /*
         *  Branch to offset if false (8 bit)
         *      BranchFalse.8
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_FALSE_8):
            opcode = (EjsOpCode) (opcode - EJS_OP_BRANCH_TRUE_8 + EJS_OP_BRANCH_TRUE);
            offset = (char) getByte(frame);
            goto commonBoolBranchCode;

        /*
         *  Branch to offset if true (8 bit)
         *      BranchTrue.8
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_TRUE_8):
            /* We want sign extension here */
            opcode = (EjsOpCode) (opcode - EJS_OP_BRANCH_TRUE_8 + EJS_OP_BRANCH_TRUE);
            offset = (char) getByte(frame);

        /*
         *  Common boolean branch code
         */
        commonBoolBranchCode:
            v1 = pop(ejs);
            if (v1 == 0 || !ejsIsBoolean(v1)) {
                v1 = ejsCastVar(ejs, v1, ejs->booleanType);
                if (ejs->exception) {
                    CHECK; BREAK;
                }
            }
            if (!ejsIsBoolean(v1)) {
                ejsThrowTypeError(ejs, "Result of a comparision must be boolean");
                CHECK; BREAK;
            }
            if (opcode == EJS_OP_BRANCH_TRUE) {
                if (((EjsBoolean*) v1)->value) {
                    frame->pc = &frame->pc[offset];
                }
            } else {
                if (((EjsBoolean*) v1)->value == 0) {
                    frame->pc = &frame->pc[offset];
                }
            }
            BREAK;

        /*
         *  Branch to offset if [value1] == null
         *      BranchNull
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_NULL):
            push(ejs, ejs->nullValue);
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Branch to offset if [value1] == undefined
         *      BranchUndefined
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_UNDEFINED):
            push(ejs, ejs->undefinedValue);
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Branch to offset if [tos] value is zero
         *      BranchZero
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_ZERO):
            /* Fall through */

        /*
         *  Branch to offset if [tos] value is not zero
         *      BranchNotZero
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_NOT_ZERO):
            //  TODO - need a pre-created zero value
            push(ejs, ejsCreateNumber(ejs, 0));
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Branch to offset if [value1] == [value2]
         *      BranchEQ
         *      Stack before (top)  [value1]
         *      Stack before (top)  [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_EQ):

        /*
         *  Branch to offset if [value1] === [value2]
         *      BranchStrictlyEQ
         *      Stack before (top)  [value1]
         *      Stack after         [value2]
         */
        CASE (EJS_OP_BRANCH_STRICTLY_EQ):

        /*
         *  Branch to offset if [value1] != [value2]
         *      BranchNotEqual
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_NE):

        /*
         *  Branch to offset if [value1] !== [value2]
         *      BranchStrictlyNE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_STRICTLY_NE):

        /*
         *  Branch to offset if [value1] <= [value2]
         *      BranchLE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_LE):

        /*
         *  Branch to offset if [value1] < [value2]
         *      BranchLT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_LT):

        /*
         *  Branch to offset if [value1] >= [value2]
         *      BranchGE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_GE):

        /*
         *  Branch to offset if [value1] > [value2]
         *      BranchGT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_GT):
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Handle all branches here. We convert to a compare opcode and pass to the type to handle.
         */
        commonBranchCode:
            opcode = (EjsOpCode) (opcode - EJS_OP_BRANCH_EQ + EJS_OP_COMPARE_EQ);
            v2 = pop(ejs);
            v1 = pop(ejs);
            result = evalBinaryExpr(ejs, v1, opcode, v2);
            if (!ejsIsBoolean(result)) {
                ejsThrowTypeError(ejs, "Result of a comparision must be boolean");
                CHECK;
            } else {
                if (((EjsBoolean*) result)->value) {
                    frame->pc = &frame->pc[offset];
                }
            }
            BREAK;

        /*
         *  Compare if [value1] == true
         *      CompareTrue
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_TRUE):

        /*
         *  Compare if ![value1]
         *      CompareNotTrue
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_FALSE):
            v1 = pop(ejs);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Compare if [value1] == NULL
         *      CompareNull
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_NULL):
            push(ejs, ejs->nullValue);
            goto binaryExpression;

        /*
         *  Compare if [item] == undefined
         *      CompareUndefined
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_UNDEFINED):
            push(ejs, ejs->undefinedValue);
            goto binaryExpression;

        /*
         *  Compare if [item] value is zero
         *      CompareZero
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_ZERO):
            push(ejs, ejsCreateNumber(ejs, 0));
            goto binaryExpression;

        /*
         *  Compare if [tos] value is not zero
         *      CompareZero
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_NOT_ZERO):
            push(ejs, ejsCreateNumber(ejs, 0));
            goto binaryExpression;

        /*
         *  Compare if [value1] == [item2]
         *      CompareEQ
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_EQ):

        /*
         *  Compare if [value1] === [item2]
         *      CompareStrictlyEQ
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_STRICTLY_EQ):

        /*
         *  Compare if [value1] != [item2]
         *      CompareNE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_NE):

        /*
         *  Compare if [value1] !== [item2]
         *      CompareStrictlyNE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_STRICTLY_NE):

        /*
         *  Compare if [value1] <= [item2]
         *      CompareLE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_LE):

        /*
         *  Compare if [value1] < [item2]
         *      CompareStrictlyLT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_LT):

        /*
         *  Compare if [value1] >= [item2]
         *      CompareGE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_GE):

        /*
         *  Compare if [value1] > [item2]
         *      CompareGT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_GT):

        /*
         *  Binary expressions
         *      Stack before (top)  [right]
         *                          [left]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_ADD):
        CASE (EJS_OP_SUB):
        CASE (EJS_OP_MUL):
        CASE (EJS_OP_DIV):
        CASE (EJS_OP_REM):
        CASE (EJS_OP_SHL):
        CASE (EJS_OP_SHR):
        CASE (EJS_OP_USHR):
        CASE (EJS_OP_AND):
        CASE (EJS_OP_OR):
        CASE (EJS_OP_XOR):
        binaryExpression:
            v2 = pop(ejs);
            v1 = pop(ejs);
            ejs->result = evalBinaryExpr(ejs, v1, opcode, v2);
            push(ejs, ejs->result);
            CHECK; BREAK;

        /*
         *  Unary operators
         */

        /*
         *  Negate a value
         *      Neg
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_NEG):
            v1 = pop(ejs);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Logical not (~value)
         *      LogicalNot
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_LOGICAL_NOT):
            v1 = pop(ejs);
            v1 = ejsCastVar(ejs, v1, ejs->booleanType);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Bitwise not (!value)
         *      BitwiseNot
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_NOT):
            v1 = pop(ejs);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Increment a stack variable
         *      Inc                 <increment>
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_INC):
            v1 = pop(ejs);
            count = (char) getByte(frame);
            result = evalBinaryExpr(ejs, v1, EJS_OP_ADD, (EjsVar*) ejsCreateNumber(ejs, count));
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Object creation
         */

        /*
         *  Create a new object
         *      New
         *      Stack before (top)  [type]
         *      Stack after         [obj]
         */
        CASE (EJS_OP_NEW):
            type = (EjsType*) pop(ejs);
            if (type == 0 || (EjsVar*) type == (EjsVar*) ejs->undefinedValue || !ejsIsType(type)) {
                ejsThrowReferenceError(ejs, "Can't locate type");
            } else {
                v1 = ejsCreateVar(ejs, type, 0);
                push(ejs, v1);
                ejs->result = v1;
            }
            CHECK; BREAK;

        /*
         *  Create a new object literal
         *      NewObject           <type> <argc.
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_NEW_OBJECT):
            //  TODO - what values need to be tested for success
            type = getType(frame);
            argc = getNum(frame);
            vp = (EjsVar*) ejsCreateObject(ejs, type, 0);
            for (i = 1 - (argc * 2); i <= 0; ) {
                nameVar = (EjsString*) ejs->stack.top[i++];
                v1 = ejs->stack.top[i++];
                if (v1 && nameVar) {
                    if (ejsIsFunction(v1)) {
                        fun = (EjsFunction*) v1;
                        if (fun->literalGetter) {
                            fun->getter = 1;
                        }
                    }
                    nameVar = ejsToString(ejs, (EjsVar*) nameVar);
                    ejsName(&qname, "", mprStrdup(vp, nameVar->value));
                    ejsSetPropertyByName(ejs, vp, &qname, v1);
                }
            }
            ejs->stack.top -= (argc * 2);
            push(ejs, vp);
            CHECK; BREAK;

        /*
         *  Reference the super class
         *      Super
         *      Stack before (top)  [obj]
         *      Stack after         [type]
         */
        CASE (EJS_OP_SUPER):
            //  TODO - not implemented
            mprAssert(0);
#if OLD && TODO
            vp = pop(ejs);
            if (ejsIsType(obj)) {
                push(ejs, ((EjsType*) obj)->baseType);
            } else {
                push(ejs, obj->type->baseType);
            }
            frame = callConstructors(ejs, type, obj, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;
#endif

        /*
         *  Delete a property
         *      Delete              <qname>
         *      Stack before (top)  [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_DELETE):
            vp = pop(ejs);
            qname = getNameArg(frame);
            slotNum = ejsDeletePropertyByName(ejs, vp, &qname);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Property \"%s\" does not exist", qname.name);
            }
            CHECK; BREAK;

        /*
         *  Delete an object property by name expression
         *      DeleteyNameExpr
         *      Stack before (top)  [obj]
         *                          [nameValue]
         *      Stack after         []
         */
        CASE (EJS_OP_DELETE_NAME_EXPR):
            nameVar = ejsToString(ejs, pop(ejs));
            vp = pop(ejs);
            ejsName(&qname, "", nameVar->value);
            slotNum = ejsLookupVar(ejs, vp, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Property \"%s\" does not exist", qname.name);
            } else {
                slotNum = ejsDeleteProperty(ejs, vp, slotNum);
            }
            CHECK; BREAK;

        /*
         *  Delete an object property from the current scope
         *      DeleteName          <name>
         *      Stack before (top)  [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_DELETE_NAME):
            qname = getNameArg(frame);
            slotNum = ejsLookupScope(ejs, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Property \"%s\" does not exist", qname.name);
            } else {
                ejsDeleteProperty(ejs, lookup.obj, slotNum);
            }
            CHECK; BREAK;


        /*
         *  No operation. Does nothing.
         *      Nop
         */
        CASE (EJS_OP_NOP):
            BREAK;

        /*
         *  Check if object is an instance of a given type. TODO - this is currently just the same as IS_A
         *      InstanceOf          <type>
         *      Stack before (top)  [type]
         *                          [obj]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_INST_OF):
            type = (EjsType*) pop(ejs);
            v1 = pop(ejs);
            push(ejs, ejsCreateBoolean(ejs, ejsIsA(ejs, v1, type)));
            BREAK;

        /*
         *  Get the type of an object.
         *      TypeOf              <obj>
         *      Stack before (top)  [obj]
         *      Stack after         [string]
         */
        CASE (EJS_OP_TYPE_OF):
            v1 = pop(ejs);
            push(ejs, ejsGetTypeOf(ejs, v1));
            BREAK;

        /*
         *  Check if object is a given type. TODO - Like should be implemented differently.
         *      IsA, Like
         *      Stack before (top)  [type]
         *                          [obj]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_IS_A):
        CASE (EJS_OP_LIKE):
            type = (EjsType*) pop(ejs);
            v1 = pop(ejs);
            push(ejs, ejsCreateBoolean(ejs, ejsIsA(ejs, v1, type)));
            BREAK;

        /*
         *  Cast an object to the given the type. Throw if not castable.
         *      Cast
         *      Stack before (top)  [type]
         *                          [obj]
         *      Stack after         [result]
         */
        CASE (EJS_OP_CAST):
            type = (EjsType*) pop(ejs);
            if (!ejsIsType(type)) {
                ejsThrowTypeError(ejs, "Not a type");
            } else {
                v1 = pop(ejs);
                push(ejs, ejsCastVar(ejs, v1, type));
            }
            CHECK; BREAK;

        /*
         *  Cast to a boolean type
         *      CastBoolean
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_CAST_BOOLEAN):
            v1 = ejsCastVar(ejs, pop(ejs), ejs->booleanType);
            push(ejs, v1);
            CHECK; BREAK;

        /*
         *  Test if a given name is the name of a property "in" an object
         *      Cast
         *      Stack before (top)  [obj]
         *                          [name]
         *      Stack after         [result]
         */
        CASE (EJS_OP_IN):
            v1 = pop(ejs);
            nameVar = ejsToString(ejs, pop(ejs));
            if (nameVar == 0) {
                ejsThrowTypeError(ejs, "Can't convert to a name");
            } else {
                ejsName(&qname, "", nameVar->value);                        //  Don't consult namespaces
                slotNum = ejsLookupProperty(ejs, v1, &qname);
                if (slotNum < 0) {
                    slotNum = ejsLookupVar(ejs, v1, &qname, 1, &lookup);
                    if (slotNum < 0 && ejsIsType(v1)) {
                        slotNum = ejsLookupVar(ejs, (EjsVar*) ((EjsType*) v1)->instanceBlock, &qname, 1, &lookup);
                    }
                }
                push(ejs, ejsCreateBoolean(ejs, slotNum >= 0));
            }
            CHECK; BREAK;

        /*
         *  Unimplemented op codes
         */
        CASE (EJS_OP_BREAKPOINT):
            mprAssert(0);
            BREAK;

        /*
         *  Create a new array literal
         *      NewArray            <type> <argc.
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_NEW_ARRAY):
            mprAssert(0);
            CHECK; BREAK;

        /*
         *  Push a qualified name constant (TODO - Not implemented)
         *      LoadName            <qname>
         *      Stack before (top)  []
         *      Stack after         [String]
         */
        CASE (EJS_OP_LOAD_NAME):
            mprAssert(0);
            qname = getNameArg(frame);
            push(ejs, ejs->undefinedValue);
            CHECK; BREAK;

        nullException:
            ejsThrowReferenceError(ejs, "Object reference is null");
            if ((frame = payAttention(ejs)) == 0) {
                return;
            }
            local = (EjsObject*) frame->currentFunction;
            BREAK;

        slotReferenceException:
            /*
             *  Callers must setup slotNum
             */
            ejsThrowReferenceError(ejs, "Property at slot \"%d\" is not found", slotNum);
            if ((frame = payAttention(ejs)) == 0) {
                return;
            }
            local = (EjsObject*) frame->currentFunction;
            BREAK;

#if !LINUX && !MACOSX && !LINUX && !SOLARIS && !VXWORKS
        }
        if (ejs->attention && (frame = payAttention(ejs)) == 0) {
            return;
        }
        local = (EjsObject*) frame->currentFunction;
    }
#endif
}



/*
 *  Attend to unusual circumstances. Memory allocation errors, exceptions and forced exits all set the attention flag.
 */
static EjsFrame *payAttention(Ejs *ejs)
{
    ejs->attention = 0;

    if (ejs->gc.required) {
        ejsCollectGarbage(ejs, EJS_GC_SMART);
    }
    if (mprHasAllocError(ejs)) {
        ejsThrowMemoryError(ejs);
        ejs->attention = 1;
        return ejs->frame;
    }
    if (ejs->exception) {
        if (!handleException(ejs)) {
            ejs->attention = 1;
            return 0;
        }
    }
    if (ejs->exception) {
        /*
         *  Unhandled exception
         */
        popFrameAndReturn(ejs);
        ejs->attention = 1;
        return 0;
    }
    if (ejs->frame->depth > EJS_MAX_RECURSION) {
        ejsThrowInternalError(ejs, "Recursion limit exceeded");
        return 0;
    }
    return ejs->frame;
}



/*
 *  Run the module initializer
 */
EjsVar *ejsRunInitializer(Ejs *ejs, EjsModule *mp)
{
    EjsModule   *dp;
    EjsVar      *result;
    int         next;

    if (mp->initialized || !mp->hasInitializer) {
        mp->initialized = 1;
        return ejs->nullValue;
    }
    mp->initialized = 1;

    if (mp->dependencies) {
        for (next = 0; (dp = (EjsModule*) mprGetNextItem(mp->dependencies, &next)) != 0;) {
            if (dp->hasInitializer && !dp->initialized) {
                if (ejsRunInitializer(ejs, dp) == 0) {
                    mprAssert(ejs->exception);
                    return 0;
                }
            }
        }
    }

    mprLog(ejs, 6, "Running initializer for module %s", mp->name);

    result = ejsRunFunction(ejs, mp->initializer, ejs->global, 0, NULL);

    ejsMakeTransient(ejs, (EjsVar*) mp->initializer);
    
    return result;
}


/*
 *  Run all initializers for all modules
 */
int ejsRun(Ejs *ejs)
{
    EjsModule   *mp;
    int         next;

    for (next = 0; (mp = (EjsModule*) mprGetNextItem(ejs->modules, &next)) != 0;) {
        if (ejsRunInitializer(ejs, mp) == 0) {
            mprAssert(ejs->exception);
            return EJS_ERR;
        }
    }
    return 0;
}


/*
 *  Run a function with the given parameters
 */
EjsVar *ejsRunFunction(Ejs *ejs, EjsFunction *fun, EjsVar *thisObj, int argc, EjsVar **argv)
{
    EjsFrame    *frame, *prev;
    int         i;
    
    mprAssert(ejs);
    mprAssert(fun);
    mprAssert(ejsIsFunction(fun));

    if (fun->thisObj) {
        /*
         *  Closure value of thisObj
         */
        thisObj = fun->thisObj;
    }

    if (ejsIsNativeFunction(fun)) {
        mprAssert(fun->body.proc);
        ejs->result = (fun->body.proc)(ejs, thisObj, argc, argv);

    } else {
        mprAssert(fun->body.code.byteCode);
        mprAssert(fun->body.code.codeLen > 0);
        
        for (i = 0; i < argc; i++) {
            push(ejs, argv[i]);
        }
        
        /*
         *  This will setup to call the function, but not interpret any byte codes yet.
         */
        prev = ejs->frame;
        frame = callFunction(ejs, fun, thisObj, argc, 0);
        if (ejs->exception == 0) {
            frame->returnFrame = 1;
            vmLoop(ejs);
            mprAssert(ejs->frame == prev);
        }
    }
    return (ejs->exception) ? 0 : ejs->result;
}


/*
 *  Run a function by slot.
 */
EjsVar *ejsRunFunctionBySlot(Ejs *ejs, EjsVar *obj, int slotNum, int argc, EjsVar **argv)
{
    EjsFunction     *fun;

    if (obj == 0) {
        mprAssert(0);
        return 0;
    }

    if (obj == ejs->global) {
        fun = (EjsFunction*) ejsGetProperty(ejs, obj, slotNum);
    } else {
        fun = (EjsFunction*) ejsGetProperty(ejs, ejsIsType(obj) ? obj : (EjsVar*) obj->type, slotNum);
    }
    if (fun == 0) {
        return 0;
    }
    return ejsRunFunction(ejs, fun, obj, argc, argv);
}


/*
 *  Validate the args. This routine handles ...rest args and parameter type checking and casts. Returns the new argc 
 *  or < 0 on errors.
 */
static int validateArgs(Ejs *ejs, EjsFunction *fun, int argc, EjsVar **argv)
{
    EjsTrait        *trait;
    EjsVar          *newArg;
    EjsArray        *rest;
    int             nonDefault, i, limit, numRest;

    nonDefault = fun->numArgs - fun->numDefault;

    if (argc < nonDefault) {
        if (!fun->rest || argc != (fun->numArgs - 1)) {
            if (fun->lang < EJS_SPEC_FIXED) {
                /*
                 *  Create undefined values for missing args
                 */
                for (i = argc; i < nonDefault; i++) {
                    push(ejs, ejs->undefinedValue);
                }
                argc = nonDefault;

            } else {
                /*
                 *  We are currently allowing too few arguments and supply "undefined" for the missing parameters
                 */
                ejsThrowArgError(ejs, "Insufficient actual parameters. Call requires %d parameter(s).", nonDefault);
                return EJS_ERR;
            }
        }
    }

    if ((uint) argc > fun->numArgs && !fun->rest) {
        /*
         *  TODO - Array match functions to Array.every take 3 parameters. Yet most users want to be able to supply fewer
         *  actual parameters and ignore the others. So let's just discard the args here.
         */
        if (1 || fun->lang < EJS_SPEC_FIXED) {
            /*
             *  TODO - currently enabled for all language spec levels.
             *  Discard excess arguments
             */
            ejs->stack.top -=  (argc - fun->numArgs);
            argc = fun->numArgs;

        } else {
            /*
             *  We are currently allowing too many arguments.
             */
            ejsThrowArgError(ejs, "Too many actual parameters. Call accepts at most %d parameter(s).", fun->numArgs);
            return EJS_ERR;
        }
    }

    /*
     *  Handle rest "..." args
     */
    if (fun->rest) {
        numRest = argc - fun->numArgs + 1;
        rest = ejsCreateArray(ejs, numRest);
        if (rest == 0) {
            return EJS_ERR;
        }
        for (i = numRest - 1; i >= 0; i--) {
            ejsSetProperty(ejs, (EjsVar*) rest, i, pop(ejs));
        }
        argc = argc - numRest + 1;
        push(ejs, rest);
    }

    if (fun->block.numTraits == 0) {
        return argc;
    }

    mprAssert((uint) fun->block.numTraits >= fun->numArgs);

    /*
     *  Cast args to the right types
     */
    limit = min((uint) argc, fun->numArgs);
    for (i = 0; i < limit; i++) {
        trait = ejsGetTrait((EjsBlock*) fun, i);
        if (trait->type && !ejsIsA(ejs, argv[i], trait->type) && argv[i] != ejs->nullValue) {
            newArg = ejsCastVar(ejs, argv[i], trait->type);
            if (newArg == 0) {
                mprAssert(ejs->exception);
                return EJS_ERR;
            }
            argv[i] = newArg;
        }
    }

    return argc;
}


/*
 *  Call a function by slot number
 */
static EjsFrame *callBySlot(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *thisObj, int argc, int stackAdjust)
{
    EjsFunction     *fun;

    fun = (EjsFunction*) ejsGetProperty(ejs, vp, slotNum);
    if (fun == 0) {
        ejsThrowInternalError(ejs, "Can't find function in slot %d", slotNum);
        return ejs->frame;
    }

    if (! ejsIsFunction(fun)) {
        ejsThrowArgError(ejs, "Variable is not a function");
        return ejs->frame;
    }

    callFunction(ejs, fun, thisObj, argc, stackAdjust);

    /*
     *  Return the current (new) frame as a convenience
     */
    return ejs->frame;
}


#if UNUSED
/*
 *  Call all constructors. Recursively build up frames for each base class constructor.
 */
static EjsFrame *callConstructors(Ejs *ejs, EjsType *type, EjsVar *thisObj, int argc)
{
    EjsType     *baseType;
    int         slotNum;

    mprAssert(type);
    mprAssert(ejsIsType(type));
    mprAssert(thisObj);

    baseType = type->baseType;

    /*
     *  Setup the top most constructor first. This will then call the next level and so on.
     *  ie. deepest constructor runs first.
     */
    if (type->hasConstructor) {
        /* 
         *  For bound types, the constructor is always the first slot (above the inherited properties) 
         */
        slotNum = type->block.numInherited;
        callBySlot(ejs, (EjsVar*) type, slotNum, thisObj, argc, 0);

    } else {
        slotNum = ejsLookupProperty(ejs, (EjsVar*) type, &type->qname);
        if (slotNum >= 0) {
            callBySlot(ejs, (EjsVar*) type, slotNum, thisObj, argc, 0);
        }
    }
    if (baseType && baseType->baseType && !type->callsSuper) {
        /* 
         *  Don't pass on args. Only call default constructors 
         */
        callConstructors(ejs, baseType, thisObj, 0);
    }
    return ejs->frame;
}
#endif


/*
 *  Call a function aka pushFunctionFrame. Supports both native and scripted functions. If native, the function is fully 
 *  invoked here. If scripted, a new frame is created and the pc adjusted to point to the new function.
 */
static EjsFrame *callFunction(Ejs *ejs, EjsFunction *fun, EjsVar *thisObj, int argc, int stackAdjust)
{
    EjsFrame        *frame, *saveFrame;
    EjsName         qname;
    EjsObject       *obj;
    EjsType         *type;
    EjsVar          **argv, *vp;
    int             numLocals, i, slotNum;

    mprAssert(fun);

    if (!ejsIsFunction(fun)) {
        /* 
         *  Handle calling a constructor to create a new instance 
         */
        if ((EjsVar*) fun == (EjsVar*) ejs->undefinedValue) {
            ejsThrowReferenceError(ejs, "Function is undefined");

        } else if (ejsIsType(fun)) {
            type = (EjsType*) fun;
            vp = ejsCreateVar(ejs, type, 0);

            if (type->hasConstructor) {
                /*
                 *  Constructor is always at slot 0 (offset by base properties)
                 */
                slotNum = type->block.numInherited;
                fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, slotNum);
                if (ejsIsNativeFunction(fun)) {
                    callFunction(ejs, fun, vp, argc, 0);
                    mprAssert(ejs->frame->returnValue);
                } else {
                    //  TODO - remove saveFrame
                    saveFrame = ejs->frame;
                    callFunction(ejs, fun, vp, argc, 0);
                    vmLoop(ejs);
                    mprAssert(ejs->frame);
                    mprAssert(ejs->frame == saveFrame);
                    ejs->frame = saveFrame;
                    ejs->frame->returnValue = vp;
                }
            } else {
                ejs->frame->returnValue = vp;
            }

        } else if (!ejsIsType(fun)) {
            ejsThrowReferenceError(ejs, "Reference is not a function");
        }
        return ejs->frame;
    }

    mprAssert(ejsIsFunction(fun));

    if (fun->thisObj) {
        /*
         *  Function has previously extracted the this value
         */
        thisObj = fun->thisObj;

    } else if (fun->staticMethod) {
        /*
         *  For static methods, must find the right base class to use as "this"
         */
        slotNum = fun->slotNum;
        type = ejsIsType(thisObj) ? (EjsType*) thisObj: thisObj->type;
        while (type) {
            /*
             *  Use baseType->numTraits rather than numInherited because implemented traits are not accessed 
             *  via the base type.
             */
            if (slotNum >= type->baseType->block.numTraits) {
                break;
            }
            type = type->baseType;
        }
        thisObj = (EjsVar*) type;
    }

    /*
     *  Validate the args. Cast to the right type, handle rest args and return with argc adjusted
     */
    argv = &ejs->stack.top[1 - argc];
    if ((argc = validateArgs(ejs, fun, argc, argv)) < 0) {
        return ejs->frame;
    }
    mprAssert(argc <= (int) fun->numArgs);

    if (ejsIsNativeFunction(fun)) {
        if (fun->body.proc == 0) {
            qname = ejsGetPropertyName(ejs, fun->owner, fun->slotNum);
            ejsThrowInternalError(ejs, "Native function \"%s\" is not defined", qname.name);
            return ejs->frame;
        }
        ejs->result = ejs->frame->returnValue = (fun->body.proc)(ejs, thisObj, argc, argv);
        ejs->stack.top = ejs->stack.top - argc - stackAdjust;

    } else {
        frame = createFrame(ejs);
        frame->function = *fun; 
        frame->function.block.obj.var.isFrame = 1;
        frame->templateBlock = (EjsBlock*) fun;
        frame->prevStackTop -= (argc + stackAdjust);
        frame->currentFunction = (EjsFunction*) &frame->function;
        frame->caller = frame->prev;
        frame->thisObj = thisObj;
        frame->code = &fun->body.code;
        frame->pc = fun->body.code.byteCode;
        setFrameDebugName(frame, fun);
        
        /* TODO - use the simpler approach in the assert */
        frame->function.block.obj.slots = (EjsVar**) ((char*) frame + MPR_ALLOC_ALIGN(sizeof(EjsFrame)));
        mprAssert(frame->function.block.obj.slots == ejs->stack.top + 1);
        mprAssert(frame->function.block.obj.slots == frame->stackBase);
        
        obj = &frame->function.block.obj;

        mprAssert(ejs->stack.top + 1 == obj->slots);
        
        /*
         *  Allow some padding to speed up creation of dynamic locals. TODO - is this worth doing?
         */
        obj->capacity += EJS_NUM_PROP;
        ejs->stack.top += obj->capacity;

        mprAssert(obj->numProp <= obj->capacity);
        mprAssert(&obj->slots[obj->capacity] == (ejs->stack.top + 1));
        
        memset(obj->slots, 0, obj->capacity * sizeof(EjsVar*));
        
        if (argc > 0) {
            frame->argc = argc;
            if ((uint) argc < (fun->numArgs - fun->numDefault) || (uint) argc > fun->numArgs) {
                ejsThrowArgError(ejs, "Incorrect number of arguments");
                return ejs->frame;
            }
            for (i = 0; i < argc; i++) {
                obj->slots[i] = argv[i];
            }
        }

        /*
         *  Initialize local vars. TODO - do this just to pickup namespaces initialization values. Loader defines those.
         *  TODO - remove this requirement.
         */
        numLocals = (obj->numProp - argc);
        if (numLocals > 0) {
            mprAssert(numLocals <= obj->numProp);
            for (i = 0; i < numLocals; i++) {
                obj->slots[i + argc] = fun->block.obj.slots[i + argc];
            }
        }
        
#if BLD_DEBUG
        frame->debugScopeChain = frame->function.block.scopeChain;
#endif
    }
    return ejs->frame;
}


/*
 *  Push a frame for lexical blocks. Used by the compiler and the VM for with and local blocks.
 */
EjsFrame *ejsPushFrame(Ejs *ejs, EjsBlock *block)
{
    EjsFrame    *frame, *prev;
    int         numProp, i;

    prev = ejs->frame;
    frame = createFrame(ejs);

    frame->function.block = *block;
    frame->templateBlock = block;

    frame->function.block.obj.var.isFrame = 1;

    setFrameDebugName(frame, block);

    if (likely(prev)) {
        /*
         *  Don't change the return caller, code or pc
         */
        frame->caller = prev->caller;
        frame->currentFunction = prev->currentFunction;
        frame->code = prev->code;
        if (prev->code) {
            frame->pc = prev->pc;
        }
        frame->exceptionArg = prev->exceptionArg;
    }

    /*
     *  Initialize properties
     */
    numProp = block->obj.numProp;
    if (numProp > 0) {
        frame->function.block.obj.slots = ejs->stack.top + 1;
        ejs->stack.top += numProp;  
        for (i = 0; i < numProp; i++) {
            frame->function.block.obj.slots[i] = block->obj.slots[i];
        }
    }
    frame->function.block.scopeChain = &prev->function.block;
    
#if BLD_DEBUG
    frame->debugScopeChain = frame->function.block.scopeChain;
#endif

    return frame;
}


/*
 *  Allocate a new frame for exceptions. We don't create new slots[] or locals. The exception frame reuses the parent slots.
 */
EjsFrame *ejsPushExceptionFrame(Ejs *ejs)
{
    EjsFrame    *frame, *prev;

    prev = ejs->frame;

    frame = createFrame(ejs);

    //  TOOD - not setting up the frame->function.block??
    
    mprAssert(prev);
    if (prev) {
        frame->function = prev->function;
        frame->templateBlock = prev->templateBlock;
        frame->code = prev->code;
        frame->currentFunction = prev->currentFunction;
        frame->caller = prev->caller;
        frame->thisObj = prev->thisObj;
    }
#if BLD_DEBUG
    frame->debugScopeChain = frame->function.block.scopeChain;
#endif

    return frame;
}


/*
 *  Pop a block frame and return to the previous frame. 
 */
EjsFrame *ejsPopFrame(Ejs *ejs)
{
    EjsFrame    *frame;

    frame = ejs->frame;
    mprAssert(frame);

    if (frame->needClosure.length > 0) {
        makeClosure(frame);
    }

    mprFreeChildren(frame);

    ejs->stack.top = frame->prevStackTop;
    ejs->frame = frame->prev;
    if (ejs->frame) {
        ejs->frame->pc = frame->pc;
    }
        
    return ejs->frame;
}


/*
 *  Pop a frame and return to the caller. This in-effect does a return from a function. This adjusts the stack and pops 
 *  off any actual arguments
 */
static bool popFrameAndReturn(Ejs *ejs)
{
    EjsFrame    *frame, *caller;

    frame = ejs->frame;
    mprAssert(frame);
    
    if (frame->needClosure.length > 0) {
        makeClosure(frame);
    }

    ejs->stack.top = frame->prevStackTop;

    if ((caller = frame->caller) != 0) {
        caller->returnValue = frame->returnValue;

        if (frame->function.getter) {
            mprFreeChildren(frame);
            push(ejs, frame->returnValue);
        
        } else {
            mprFreeChildren(frame);
        }
    } else {
        mprFreeChildren(frame);
    }
    if ((ejs->frame = caller) == 0) {
        return 1;
    }
    return frame->returnFrame;
}


/*
 *  Pop an exception frame and return to the previous frame. This in-effect does a return from a catch/finally block.
 */
static EjsFrame *popExceptionFrame(Ejs *ejs)
{
    EjsFrame        *frame, *prev;

    frame = ejs->frame;
    mprAssert(frame);

    if (frame->needClosure.length > 0) {
        makeClosure(frame);
    }

    prev = frame->prev;

    if (prev) {
        prev->returnValue = frame->returnValue;

        if (ejs->exception == 0) {
            ejs->exception = prev->saveException;
            prev->saveException = 0;
            if (ejs->exception) {
                ejs->attention = 1;
            }
        }
    }
    ejs->stack.top = frame->prevStackTop;

    mprFreeChildren(frame);

    /*
     *  Update the current frame. If the current frame is a function, this is actually a return.
     */
    ejs->frame = prev;

    return prev;
}


/*
 *  Allocate a new frame. Frames are created for functions and blocks.
 */
static inline EjsFrame *createFrame(Ejs *ejs)
{
    EjsFrame    *frame;
    int         size;

    size = MPR_ALLOC_ALIGN(MPR_ALLOC_HDR_SIZE + sizeof(EjsFrame));

    /*
     *  Allocate the frame off the eval stack.
     */
    frame = (EjsFrame*) (((char*) (ejs->stack.top + 1)) + MPR_ALLOC_HDR_SIZE);

    /*
     *  Initialize the memory header. This allows "frame" to be used as a memory context for mprAlloc
     */
    mprInitBlock(ejs, frame, size);
    
    if (likely(ejs->frame)) {
        frame->depth = ejs->frame->depth + 1;
    }
    frame->prev = ejs->frame;
    frame->ejs = ejs;
    frame->prevStackTop = ejs->stack.top;
    frame->thisObj = ejs->global;

    ejs->frame = frame;
    ejs->stack.top += (size / sizeof(EjsVar*));
    frame->stackBase = ejs->stack.top + 1;

    frame->function.block.obj.var.isFrame = 1;

    /*
     *  TODO OPT - bit fields
     */
    frame->function.block.obj.var.generation = 0;
    frame->function.block.obj.var.rootLinks = 0;
    frame->function.block.obj.var.refLinks = 0;
    frame->function.block.obj.var.builtin = 0;
    frame->function.block.obj.var.dynamic = 0;
    frame->function.block.obj.var.hasGetterSetter = 0;
    frame->function.block.obj.var.isFunction = 0;
    frame->function.block.obj.var.isObject = 0;
    frame->function.block.obj.var.isInstanceBlock = 0;
    frame->function.block.obj.var.isType = 0;
    frame->function.block.obj.var.isFrame = 0;
    frame->function.block.obj.var.hidden = 0;
    frame->function.block.obj.var.marked = 0;
    frame->function.block.obj.var.native = 0;
    frame->function.block.obj.var.nativeProc = 0;
    frame->function.block.obj.var.permanent = 0;
    frame->function.block.obj.var.survived = 0;
    frame->function.block.obj.var.visited = 0;
    
    return frame;
}


/*
 *  Called for catch and finally blocks
 */
static void callExceptionHandler(Ejs *ejs, EjsFunction *fun, int index, int flags)
{
    EjsFrame    *frame;
    EjsEx       *ex, *thisEx;
    EjsCode     *code;
    uint        handlerEnd;
    int         i;

    mprAssert(fun);

    frame = ejs->frame;
    code = &fun->body.code;
    mprAssert(0 <= index && index < code->numHandlers);
    ex = code->handlers[index];
    mprAssert(ex);

    if (flags & EJS_EX_ITERATION) {
        /*
         *  Empty handler is a special case for iteration. We simply do a break to the handler location
         *  which targets the end of the for/in loop.
         */
        ejs->frame->pc = &frame->code->byteCode[ex->handlerStart];
        ejs->exception = 0;
        return;
    }

    /*
     *  Allocate a new frame in which to execute the handler
     */
    frame = ejsPushExceptionFrame(ejs);
    if (frame == 0) {
        /*  Exception will continue to bubble up */
        return;
    }

    /*
     *  Move the PC outside of the try region. If this is a catch block, this allows the finally block to still
     *  be found. But if this is processing a finally block, the scanning for a matching handler will be forced
     *  to bubble up.
     */
    frame->pc = &frame->code->byteCode[ex->handlerStart];

    if (flags & EJS_EX_CATCH) {
        mprAssert(frame->inFinally == 0);
        frame->inCatch = 1;
        frame->ex = ex;
        frame->exceptionArg = ejs->exception;
        // push(ejs, ejs->exception);

    } else {
        mprAssert(flags & EJS_EX_FINALLY);
        mprAssert(frame->inCatch == 0);
        frame->inFinally = 1;
        frame->ex = ex;

        /*
         *  Mask the exception while processing the finally block
         */
        mprAssert(frame->saveException == 0);
        frame->prev->saveException = ejs->exception;
        ejs->attention = 1;
    }

    /*
     *  Adjust the PC in the original frame to be outside the try block so that we don't come back to here if the
     *  catch block does handle it.
     */
    frame->prev->pc = &frame->prev->code->byteCode[ex->handlerEnd - 1];
    ejs->exception = 0;

    /*
     *  Find the end of the exception block.
     *  TODO OPT - how could the compiler (simply) speed this up?
     */
    handlerEnd = 0;
    for (i = index; i < code->numHandlers; i++) {
        thisEx = code->handlers[i];
        if (ex->tryEnd == thisEx->tryEnd) {
            if (thisEx->handlerEnd > handlerEnd) {
                handlerEnd = thisEx->handlerEnd;
            }
        }
    }
    mprAssert(handlerEnd > 0);
    frame->prev->endException = &code->byteCode[handlerEnd];
}


/*
 *  Search for an exception handler at this level to process the exception. Return true if the exception is handled.
 */
static void handleExceptionAtThisLevel(Ejs *ejs, EjsFrame *frame)
{
    EjsFunction *fun;
    EjsCode     *code;
    EjsEx       *ex;
    uint        pc;
    int         i;

    ex = 0;

    code = frame->code;
    if (code == 0 || code->numHandlers == 0) {
        return;
    }

    /*
     *  The PC is always one advanced from the throwing instruction. ie. the PC has probably advanced
     *  past the instruction and now points to the next instruction. So reverse by one.
     */
    pc = (uint) (frame->pc - code->byteCode - 1);
    mprAssert(pc >= 0);

    if (frame->inCatch == 0 && frame->inFinally == 0) {
        /*
         *  Normal exception in a try block. NOTE: the catch will jump or fall through to the finally block code.
         *  ie. We won't come here again for the finally code unless there is an exception in the catch block.
         */
        for (i = 0; i < code->numHandlers; i++) {
            ex = code->handlers[i];
            mprAssert(ex);
            if (ex->tryStart <= pc && pc < ex->tryEnd && ex->flags & EJS_EX_CATCH) {
                //  TODO - temp comparison to voidType
                if (ex->catchType == ejs->voidType || ejsIsA(ejs, (EjsVar*) ejs->exception, ex->catchType)) {
                    fun = frame->currentFunction;
                    callExceptionHandler(ejs, fun, i, ex->flags);
                    return;
                }
            }
        }
        /*
         *  No catch handler at this level. Bubble up. But first invoke any finally handler -- see below.
         */

    } else {
        /*
         *  Exception in a catch block or in a finally block. If in a catch block, must first run the finally
         *  block before bubbling up. If in a finally block, we are done and upper levels will handle. We can be
         *  in a finally block and inFinally == 0. This happens because catch blocks jump or fall through directly
         *  into finally blocks (fast). But we need to check here if we are in the finally block explicitly.
         */
        for (i = 0; i < code->numHandlers; i++) {
            ex = code->handlers[i];
            mprAssert(ex);
            if (ex->handlerStart <= pc && pc < ex->handlerEnd && ex->flags & EJS_EX_FINALLY) {
                frame->inFinally = 1;
                break;
            }
        }
        if (frame->inFinally) {
            /*
             *  If falling through from a catch code block into a finally code block, we must push the outer blocks's
             *  frame pc to be outside the tryStart to finallyStart region. This prevents this try block from
             *  handling this exception again.
             */
            frame->prev->pc = &frame->prev->code->byteCode[ex->handlerEnd - 1];
            return;
        }
    }

    /*
     *  Either exception in the catch handler or exception not handled by this frame. Before returning up the stack
     *  to find upper handler to process the exception, we must first invoke the finally handler at this level if
     *  one exists.
     */
    mprAssert(!frame->inFinally);
    for (i = 0; i < code->numHandlers; i++) {
        ex = code->handlers[i];
        mprAssert(ex);
        /*
         *  Go from try start to finally start incase there is an exception in a catch block.
         */
        if (ex->tryStart <= pc && pc < ex->handlerStart && ex->flags & EJS_EX_FINALLY) {
            /*
             *  Clear any old catch frame
             */
            if (frame->inCatch) {
                frame = popExceptionFrame(ejs);
            }
            /*
             *  Create a finally block. Only case here is for exceptions in the try or catch regions.
             */
            fun = frame->currentFunction;
            callExceptionHandler(ejs, fun, i, EJS_EX_FINALLY);
            break;
        }
    }
}


/*
 *  Process and exception. Bubble up the exception until we find an exception handler for it.
 */
static bool handleException(Ejs *ejs)
{
    EjsFrame    *frame;

    /*
     *  Check at each level for a handler to process the exception.
     */
    for (frame = ejs->frame; frame; frame = popExceptionFrame(ejs)) {
        handleExceptionAtThisLevel(ejs, frame);
        if (ejs->exception == 0) {
            return 1;
        }
        if (frame->returnFrame) {
            popExceptionFrame(ejs);
            return 0;
        }
    }
    return 0;
}


typedef struct OperMap {
    int         opcode;
    cchar       *name;
} OperMap;

static OperMap operMap[] = {
        { EJS_OP_MUL,           "*"     },
        { EJS_OP_DIV,           "/"     },
        { EJS_OP_REM,           "%"     },
        { EJS_OP_COMPARE_LT,    "<"     },
        { EJS_OP_COMPARE_GT,    ">"     },
        { EJS_OP_COMPARE_LE,    "<="    },
        { EJS_OP_COMPARE_GE,    ">="    },
        { 0,                    0       },
};


static int lookupOverloadedOperator(Ejs *ejs, EjsOpCode opcode, EjsVar *lhs)
{
    EjsName     qname;
    int         i;

    for (i = 0; operMap[i].opcode; i++) {
        if (operMap[i].opcode == opcode) {
            ejsName(&qname, "", operMap[i].name);
            break;
        }
    }
    return ejsLookupProperty(ejs, (EjsVar*) lhs->type, &qname);
}


/*
 *  Evaluate a binary expression.
 *  TODO -- simplify and move back inline into eval loop.
 */
static EjsVar *evalBinaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode, EjsVar *rhs)
{
    EjsVar      *result;
    int         slotNum;

    if (lhs == 0) {
        lhs = ejs->undefinedValue;
    }
    if (rhs == 0) {
        rhs = ejs->undefinedValue;
    }

    result = ejsInvokeOperator(ejs, lhs, opcode, rhs);

    if (result == 0 && ejs->exception == 0) {
        slotNum = lookupOverloadedOperator(ejs, opcode, lhs);
        if (slotNum >= 0) {
            result = ejsRunFunctionBySlot(ejs, lhs, slotNum, 1, &rhs);
        }
    }
    return result;
}


/*
 *  Evaluate a unary expression.
 *  TODO -- once simplified, move back inline into eval loop.
 */
static EjsVar *evalUnaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode)
{
    return ejsInvokeOperator(ejs, lhs, opcode, 0);
}


int ejsInitStack(Ejs *ejs)
{
    EjsStack    *stack;

    /*
     *  Allocate the stack
     */
    stack = &ejs->stack;
    stack->size = MPR_PAGE_ALIGN(EJS_STACK_MAX, mprGetPageSize(ejs));

    /*
     *  This will allocate memory virtually for systems with virutal memory. Otherwise, it will just use malloc.
     *  TODO - should create a guard page to catch stack overflow.
     */
    stack->bottom = mprMapAlloc(stack->size, MPR_MAP_READ | MPR_MAP_WRITE);
    if (stack->bottom == 0) {
        mprSetAllocError(ejs);
        return EJS_ERR;
    }
    stack->top = &stack->bottom[-1];

    return 0;
}


#if FUTURE
/*
 *  Grow the operand evaluation stack.
 *  Return a negative error code on memory allocation errors or if the stack grows too big.
 */
int ejsGrowStack(Ejs *ejs, int incr)
{
    EjsStack *sp;
    EjsFrame *frame;
    EjsVar **bottom;
    int i, size, moveBy;

    sp = ejs->stack;
    sp->ejs = ejs;

    incr = max(incr, EJS_STACK_INC);

    if (sp->bottom) {
        /*
         *  Grow an existing stack
         */
        size = sp->size + (sizeof(EjsVar*) * incr);
        bottom = (EjsVar**) mprRealloc(sp, sp->bottom, size);
        //  TODO - do we really need zeroed?
        memset(&bottom[sp->size], 0, (size - sp->size) * sizeof(EjsVar*));
        moveBy = (int) ((char*) bottom - (char*) sp->bottom);
        sp->top = (EjsVar**) ((char*) sp->top + moveBy);
        sp->bottom = bottom;

        /*
         *  Adjust all the argv pointers. TODO REFACTOR!!!!
         */
        for (frame = ejs->frame; frame; frame = frame->prev) {
            if (frame->argv) {
                frame->argv = (EjsVar**) ((char*) frame->argv + moveBy);
            }
            frame->prevStackTop = (EjsVar**) ((char*) frame->prevStackTop + moveBy);
        }

    } else {
        /*
         *  Allocate a stack
         */
        if (sp->top >= &sp->bottom[EJS_STACK_MAX]) {
            return MPR_ERR_NO_MEMORY;
        }
        size = (sizeof(EjsVar*) * incr);
        sp->bottom = (EjsVar**) mprAlloc(sp, size);
        /*
         *  Push always begins with an increment of sp->top. Initially, sp_bottom points to the first (future) element.
         */
        sp->top = &sp->bottom[-1];
    }

    if (sp->bottom == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    sp->end = &sp->bottom[size / sizeof(EjsVar*)];
    sp->size = size;

    //  TODO - opt memset
    for (i = 1; i <= incr; i++) {
        sp->top[i] = 0;
    }

    return 0;
}
#endif


/*
 *  Exit the script
 */
void ejsExit(Ejs *ejs, int status)
{
    //  TODO - should pass status back
    ejs->flags |= EJS_FLAG_EXIT;
}


/*
 *  Get an encoded number. Relies on correct byte code and some form of validation.
 */
static int getNum(EjsFrame *frame)
{
    uint t, c;

    c = (uint) *frame->pc++;
    t = c & 0x7f;

    if (c & 0x80) {
        c = (uint) *frame->pc++;
        t |= ((c & 0x7f) << 7);

        if (c & 0x80) {
            c = (uint) *frame->pc++;
            t |= ((c & 0x7f) << 14);

            if (c & 0x80) {
                c = (uint) *frame->pc++;
                t |= ((c & 0x7f) << 21);

                if (c & 0x80) {
                    c = (uint) *frame->pc++;
                    t |= ((c & 0x7f) << 28);
                }
            }
        }
    }
    return (int) t;
}


static EjsName getNameArg(EjsFrame *frame)
{
    EjsName qname;

    qname.name = getStringArg(frame);
    qname.space = getStringArg(frame);

    return qname;
}


/*
 *  Get an interned string. String constants are stored as token offsets into the constant pool. The pool
 *  contains null terminated UTF-8 strings.
 */
static char *getStringArg(EjsFrame *frame)
{
    int number;

    number = getNum(frame);
    if (number < 0) {
        ejsThrowInternalError(frame->ejs, "Bad instruction token");
        return 0;
    }

    mprAssert(frame->code->constants);
    return &frame->code->constants->pool[number];
}


static EjsVar *getGlobalArg(EjsFrame *frame)
{
    Ejs         *ejs;
    EjsVar      *vp;
    EjsName     qname;
    int         t, slotNum;

    ejs = frame->ejs;

    t = getNum(frame);
    if (t < 0) {
        return 0;
    }

    slotNum = -1;
    qname.name = 0;
    qname.space = 0;
    vp = 0;

    /*
     *  TODO - OPT. Could this encoding be optimized?
     */
    switch (t & EJS_ENCODE_GLOBAL_MASK) {
    default:
        mprAssert(0);
        return 0;

    case EJS_ENCODE_GLOBAL_NOREF:
        return 0;

    case EJS_ENCODE_GLOBAL_SLOT:
        slotNum = t >> 2;
        if (0 <= slotNum && slotNum < ejsGetPropertyCount(ejs, ejs->global)) {
            vp = ejsGetProperty(ejs, ejs->global, slotNum);
        }
        break;

    case EJS_ENCODE_GLOBAL_NAME:
        qname.name = &frame->code->constants->pool[t >> 2];
        if (qname.name == 0) {
            mprAssert(0);
            return 0;
        }
        qname.space = getStringArg(frame);
        if (qname.space == 0) {
            return 0;
        }
        if (qname.name) {
            vp = ejsGetPropertyByName(ejs, ejs->global, &qname);
        }
        break;
    }
    return vp;
}


/*
 *  Get a function reference. This binds the "this" value for method extraction. Also handles getters.
 */
static EjsFrame *getFunction(Ejs *ejs, EjsVar *thisObj, EjsVar *obj, int slotNum, EjsFunction *fun, EjsObject **local)
{
    EjsFrame    *frame;

    frame = ejs->frame;

    if (fun->getter) {
        callFunction(ejs, fun, (thisObj) ? thisObj : obj, 0, 0);
        if (ejsIsNativeFunction(fun)) {
            push(ejs, frame->returnValue);
        }

    } else if (slotNum == fun->slotNum && !fun->block.obj.var.nativeProc) {
        if (obj == fun->owner || (ejsIsType(obj) && ejsIsType(fun->owner) && ejsIsA(ejs, obj, (EjsType*) fun->owner))) {
            
            /*
             *  Bind "this" for the function
             */
            if (thisObj == 0) {
                if (obj == (EjsVar*) &frame->function) {
                    thisObj = frame->thisObj;

                } else if (fun->thisObj) {
                    thisObj = fun->thisObj;

                } else {
                    thisObj = obj;
                }
            }

            fun = ejsCopyFunction(ejs, fun);
            if (fun == 0) {
                *local = (EjsObject*) frame->currentFunction;
                return ejs->frame;
            }
            fun->thisObj = thisObj;

            if (fun->fullScope) {
                needClosure(frame, (EjsBlock*) fun);
            }
        }
        push(ejs, fun);

    } else {
        push(ejs, fun);
    }

    if (unlikely(ejs->attention)) {
        payAttention(ejs);
    }

    if (ejs->frame) {
        *local = (EjsObject*) ejs->frame->currentFunction;
    }
    return ejs->frame;
}


/*
 *  Handle setters. Setters, if present, are chained off the getter.
 */
static void putFunction(Ejs *ejs, EjsVar *thisObj, EjsFunction *fun, EjsVar *value)
{
    mprAssert(fun->getter && fun->nextSlot);
    fun = (EjsFunction*) ejsGetProperty(ejs, fun->owner, fun->nextSlot);
    mprAssert(fun && ejsIsFunction(fun) && fun->setter);
    ejsRunFunction(ejs, fun, thisObj, 1, &value);
}


/*
 *  Store a property by name somewhere in the current scope chain. Will create properties if the given name does not
 *  already exist.
 */
static void storePropertyToScope(Ejs *ejs, EjsName *qname)
{
    EjsFunction     *fun;
    EjsFrame        *frame;
    EjsVar          *value, **vpp, *obj;
    EjsObject       *block;
    EjsLookup       lookup;
    int             slotNum;

    mprAssert(qname);

    frame = ejs->frame;

    slotNum = ejsLookupScope(ejs, qname, 1, &lookup);

    if (slotNum >= 0) {
        obj = lookup.obj;

        slotNum = ejsLookupScope(ejs, qname, 1, &lookup);

        fun = (EjsFunction*) ejsGetProperty(ejs, obj, slotNum);
        if (fun && ejsIsFunction(fun) && (fun->getter || fun->setter)) {
            /*
             *  Handle setters. Setters, if present, are chained off the getter.
             */
            if (fun->getter && fun->nextSlot) {
                fun = (EjsFunction*) ejsGetProperty(ejs, fun->owner, fun->nextSlot);
                mprAssert(fun && ejsIsFunction(fun) && fun->setter);
            }
            callFunction(ejs, fun, obj, 1, 0);
            ejs->attention = 1;
            return;
        }

    } else if (frame->currentFunction->lang & EJS_SPEC_FIXED && frame->caller) {
        /*
         *  Create a new local property instead of a global
         */
        obj = (EjsVar*) &frame->function.block;
        block = &frame->function.block.obj;
        
        if (block->numProp >= block->capacity) {
            /*
             *  Copy up the evaluation stack to make room for the var
             */
            vpp = &block->slots[block->capacity];
            for (vpp = ejs->stack.top; vpp >= &block->slots[block->capacity]; vpp--) {
                vpp[EJS_NUM_PROP] = vpp[0];
            }
            block->capacity += EJS_NUM_PROP;
            ejs->stack.top += EJS_NUM_PROP;
        }

    } else {
        obj = ejs->global;
    }

    value = pop(ejs);
    ejs->result = value;

    slotNum = ejsSetProperty(ejs, obj, slotNum, value);
    if (slotNum >= 0) {
        ejsSetPropertyName(ejs, obj, slotNum, qname);
    }
}


/*
 *  Store a property by name in the given object. Will create if the property does not already exist.
 */
static void storeProperty(Ejs *ejs, EjsVar *obj, EjsName *qname)
{
    EjsFrame        *frame;
    EjsFunction     *fun;
    EjsLookup       lookup;
    EjsVar          *value, *origObj;
    int             slotNum;

    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(obj);

    frame = ejs->frame;
    slotNum = -1;
    origObj = obj;

    ejs->result = value = pop(ejs);

    if (obj->type->helpers->setPropertyByName) {
        /*
         *  If a setPropertyByName helper is defined, defer to it. Allows types like XML to not offer slot based APIs.
         */
        slotNum = (*obj->type->helpers->setPropertyByName)(ejs, obj, qname, value);
        if (slotNum >= 0) {
            ejsSetReference(ejs, obj, value);
            return;
        }
    }

    slotNum = ejsLookupVar(ejs, obj, qname, 1, &lookup);

    if (slotNum >= 0) {
        obj = lookup.obj;

        /*
         *  Handle setters. Setters, if present, are chained off the getter.
         */
        if (obj->hasGetterSetter) {
            fun = (EjsFunction*) ejsGetProperty(ejs, obj, slotNum);
            if (ejsIsFunction(fun) && (fun->getter || fun->setter)) {
                if (fun->getter && fun->nextSlot) {
                    fun = (EjsFunction*) ejsGetProperty(ejs, fun->owner, fun->nextSlot);
                    mprAssert(fun && ejsIsFunction(fun) && fun->setter);
                }
                push(ejs, value);
                callFunction(ejs, fun, origObj, 1, 0);
                ejs->attention = 1;
                return;
            }
        }
    }

    slotNum = ejsSetProperty(ejs, obj, slotNum, value);
    if (slotNum >= 0) {
        /* TODO - OPT. This can be improved. Don't always need to set the name */
        ejsSetPropertyName(ejs, obj, slotNum, qname);
    }
}


static void needClosure(EjsFrame *frame, EjsBlock *block)
{
    block->scopeChain = (EjsBlock*) frame;
    ejsAddItem(frame, &frame->needClosure, block);
}


static void makeClosure(EjsFrame *frame)
{
    EjsBlock    *block, *closure, *b;
    int         next;

    mprLog(frame, 7, "Making closure for %d blocks", frame->needClosure.length);

    closure = 0;
    next = 0;
    while ((block = ejsGetNextItem(&frame->needClosure, &next)) != 0) {
        /*
         *  Search the existing scope chain to find this frame. Then replace it.
         */
        for (b = block; b; b = b->scopeChain) {
            if (b->scopeChain == (EjsBlock*) frame) {
                if (closure == 0) {
                    /*
                     *  TODO - OPT. If returning from a frame (always), we could steal the namespace list. 
                     *  So ejsCloneVar may not be the fastest way to do this.
                     */
                    closure = (EjsBlock*) ejsCloneVar(frame->ejs, (EjsVar*) frame, 0);
                }
                b->scopeChain = closure;
                ejsSetReference(frame->ejs, (EjsVar*) b, (EjsVar*) closure);
                break;
            }
        }

        /*
         *  Pass these blocks needing closures up to the previous block as it will need to patch/replace itself in the
         *  closure chain when it returns.
         */
        if (frame->prev) {
            /*
             *  TODO BUG. This is processing the block for unrelated call-chain functions. Should only do lexical scope.
             *  ie. the top level frame and any exception blocks.
             */
            ejsAddItem(frame->prev, &frame->prev->needClosure, block);
        }
    }
}


static void swap2(Ejs *ejs)
{
    EjsVar      *tmp;

    tmp = ejs->stack.top[0];
    ejs->stack.top[0] = ejs->stack.top[-1];
    ejs->stack.top[-1] = tmp;
}


static void debug(EjsFrame *frame)
{
    int lastLine;

    lastLine = frame->lineNumber;

    frame->fileName = getStringArg(frame);
    frame->lineNumber = getNum(frame);
    frame->currentLine = getStringArg(frame);
}


static void throwNull(Ejs *ejs)
{
    ejsThrowReferenceError(ejs, "Object reference is null");
}


static EjsVar *getNthBase(Ejs *ejs, EjsVar *obj, int nthBase)
{
    EjsType     *type;

    if (obj) {
        if (ejsIsType(obj) || obj == ejs->global) {
            type = (EjsType*) obj;
        } else {
            type = obj->type;
            nthBase--;
        }
        for (; type && nthBase > 0; type = type->baseType) {
            nthBase--;
        }
        obj = (EjsVar*) type;
    }
    return obj;
}


static EjsVar *getNthBaseFromBottom(Ejs *ejs, EjsVar *obj, int nthBase)
{
    EjsType     *type, *tp;
    int         count;

    if (obj) {
        if (ejsIsType(obj) || obj == ejs->global) {
            type = (EjsType*) obj;
        } else {
            type = obj->type;
        }
        for (count = 0, tp = type->baseType
             ; tp; tp = tp->baseType) {
            count++;
        }
        nthBase = count - nthBase;
        for (; type && nthBase > 0; type = type->baseType) {
            nthBase--;
        }
        obj = (EjsVar*) type;
    }
    return obj;
}


static inline EjsVar *getNthBlock(EjsFrame *frame, int nth)
{
    EjsBlock    *block;

    mprAssert(frame);
    mprAssert(nth >= 0);

    block = &frame->function.block;

    while (block && --nth >= 0) {
        block = block->scopeChain;
    }
    mprAssert(block != frame->ejs->globalBlock);
    return (EjsVar*) block;
}


static EjsBlock *varToBlock(Ejs *ejs, EjsVar *vp)
{
    EjsBlock    *block;
    
    block = ejsCreateBlock(ejs, "", 0);
    if (block == 0) {
        return 0;
    }
    memcpy((void*) block, vp, vp->type->instanceSize);
    return block;
}


/*
 *  Enter a mesage into the log file
 */
void ejsLog(Ejs *ejs, const char *fmt, ...)
{
    va_list     args;
    char        buf[MPR_MAX_LOG_STRING];
    int         len;

    va_start(args, fmt);
    len = mprVsprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);

    mprLog(ejs, 0, buf, len);

    va_end(args);
}


void ejsShowStack(Ejs *ejs)
{
    char    *stack;
    
    stack = ejsFormatStack(ejs);
    mprLog(ejs, 7, "Stack\n%s", stack);
    mprFree(stack);
}


#if BLD_DEBUG || 1
static char *opcodes[] = {
    "Add",
    "AddNamespace",
    "AddNamespaceRef",
    "And",
    "BranchEQ",
    "BranchStrictlyEQ",
    "BranchFalse",
    "BranchGE",
    "BranchGT",
    "BranchLE",
    "BranchLT",
    "BranchNE",
    "BranchStrictlyNE",
    "BranchNull",
    "BranchNotZero",
    "BranchTrue",
    "BranchUndefined",
    "BranchZero",
    "BranchFalse.8",
    "BranchTrue.8",
    "Breakpoint",
    "Call",
    "CallGlobalSlot",
    "CallObjSlot",
    "CallThisSlot",
    "CallBlockSlot",
    "CallObjInstanceSlot",
    "CallObjStaticSlot",
    "CallThisStaticSlot",
    "CallName",
    "CallScopedName",
    "CallConstructor",
    "CallNextConstructor",
    "Cast",
    "CastBoolean",
    "CloseBlock",
    "CloseWith",
    "CompareEQ",
    "CompareStrictlyEQ",
    "CompareFalse",
    "CompareGE",
    "CompareGT",
    "CompareLE",
    "CompareLT",
    "CompareNE",
    "CompareStrictlyNE",
    "CompareNull",
    "CompareNotZero",
    "CompareTrue",
    "CompareUndefined",
    "CompareZero",
    "Debug",
    "DefineClass",
    "DefineFunction",
    "DefineGlobalFunction",
    "DeleteNameExp",
    "Delete",
    "DeleteName",
    "Div",
    "Dup",
    "Dup2",
    "EndCode",
    "EndException",
    "Goto",
    "Goto.8",
    "Inc",
    "InitDefaultArgs",
    "InitDefaultArgs.8",
    "InstOf",
    "IsA",
    "Load0",
    "Load1",
    "Load2",
    "Load3",
    "Load4",
    "Load5",
    "Load6",
    "Load7",
    "Load8",
    "Load9",
    "LoadDouble",
    "LoadFalse",
    "LoadGlobal",
    "LoadInt.16",
    "LoadInt.32",
    "LoadInt.64",
    "LoadInt.8",
    "LoadM1",
    "LoadName",
    "LoadNamespace",
    "LoadNull",
    "LoadRegexp",
    "LoadString",
    "LoadThis",
    "LoadTrue",
    "LoadUndefined",
    "LoadXML",
    "GetLocalSlot_0",
    "GetLocalSlot_1",
    "GetLocalSlot_2",
    "GetLocalSlot_3",
    "GetLocalSlot_4",
    "GetLocalSlot_5",
    "GetLocalSlot_6",
    "GetLocalSlot_7",
    "GetLocalSlot_8",
    "GetLocalSlot_9",
    "GetObjSlot_0",
    "GetObjSlot_1",
    "GetObjSlot_2",
    "GetObjSlot_3",
    "GetObjSlot_4",
    "GetObjSlot_5",
    "GetObjSlot_6",
    "GetObjSlot_7",
    "GetObjSlot_8",
    "GetObjSlot_9",
    "GetThisSlot_0",
    "GetThisSlot_1",
    "GetThisSlot_2",
    "GetThisSlot_3",
    "GetThisSlot_4",
    "GetThisSlot_5",
    "GetThisSlot_6",
    "GetThisSlot_7",
    "GetThisSlot_8",
    "GetThisSlot_9",
    "GetName",
    "GetObjName",
    "GetObjNameExpr",
    "GetBlockSlot",
    "GetGlobalSlot",
    "GetLocalSlot",
    "GetObjSlot",
    "GetThisSlot",
    "GetTypeSlot",
    "GetThisTypeSlot",
    "In",
    "Like",
    "LogicalNot",
    "Mul",
    "Neg",
    "New",
    "NewArray",
    "NewObject",
    "Nop",
    "Not",
    "OpenBlock",
    "OpenWith",
    "Or",
    "Pop",
    "PopItems",
    "PushCatchArg",
    "PushResult",
    "PutLocalSlot_0",
    "PutLocalSlot_1",
    "PutLocalSlot_2",
    "PutLocalSlot_3",
    "PutLocalSlot_4",
    "PutLocalSlot_5",
    "PutLocalSlot_6",
    "PutLocalSlot_7",
    "PutLocalSlot_8",
    "PutLocalSlot_9",
    "PutObjSlot_0",
    "PutObjSlot_1",
    "PutObjSlot_2",
    "PutObjSlot_3",
    "PutObjSlot_4",
    "PutObjSlot_5",
    "PutObjSlot_6",
    "PutObjSlot_7",
    "PutObjSlot_8",
    "PutObjSlot_9",
    "PutThisSlot_0",
    "PutThisSlot_1",
    "PutThisSlot_2",
    "PutThisSlot_3",
    "PutThisSlot_4",
    "PutThisSlot_5",
    "PutThisSlot_6",
    "PutThisSlot_7",
    "PutThisSlot_8",
    "PutThisSlot_9",
    "PutObjNameExpr",
    "PutScopedNameExpr",
    "PutObjName",
    "PutScopedName",
    "PutBlockSlot",
    "PutGlobalSlot",
    "PutLocalSlot",
    "PutObjSlot",
    "PutThisSlot",
    "PutTypeSlot",
    "PutThisTypeSlot",
    "Rem",
    "Return",
    "ReturnValue",
    "SaveResult",
    "Shl",
    "Shr",
    "Sub",
    "Super",
    "Swap",
    "Throw",
    "Ushr",
    "Xor",
    "Ext",
    0
};


static EjsOpCode traceCode(Ejs *ejs, EjsOpCode opcode)
{
    EjsFrame    *frame;
    int         len;
    static int  once = 0;
    static int  stop = 1;

    frame = ejs->frame;

    if (ejs->initialized && opcode != EJS_OP_DEBUG) {
        //  TODO - should strip '\n' in the compiler
        if (frame->currentLine) {
            len = (int) strlen(frame->currentLine) - 1;
            if (frame->currentLine[len] == '\n') {
                ((char*) frame->currentLine)[len] = '\0';
            }
        }
        //  TODO - compiler should strip '\n' from currentLine and we should explicitly add it here
        mprLog(ejs, 6, "%04d: [%d] %02x: %-35s # %s:%d %s",
            (uint) (frame->pc - frame->code->byteCode) - 1, (int) (ejs->stack.top - frame->stackBase + 1),
            (uchar) opcode, opcodes[opcode], frame->fileName, frame->lineNumber, frame->currentLine);
        if (stop && once++ == 0) {
             mprSleep(ejs, 0);
        }
    }
    return opcode;
}
#endif


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsInterp.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsList.c"
 */
/************************************************************************/

/**
 *  ejsList.c - Simple static list type.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int growList(MprCtx ctx, EjsList *lp, int incr);

#define CAPACITY(lp) (mprGetBlockSize(lp) / sizeof(void*))

//  TODO - inline some of these functions as macros
//
/*
 *  Initialize a list which may not be a memory context.
 */
void ejsInitList(EjsList *lp)
{
    lp->length = 0;
    lp->maxSize = MAXINT;
    lp->items = 0;
}


/*
 *  Define the list maximum size. If the list has not yet been written to, the initialSize will be observed.
 */
int ejsSetListLimits(MprCtx ctx, EjsList *lp, int initialSize, int maxSize)
{
    int         size;

    if (initialSize <= 0) {
        initialSize = MPR_LIST_INCR;
    }
    if (maxSize <= 0) {
        maxSize = MAXINT;
    }
    size = initialSize * sizeof(void*);

    if (lp->items == 0) {
        lp->items = (void**) mprAllocZeroed(ctx, size);

        if (lp->items == 0) {
            mprFree(lp);
            return MPR_ERR_NO_MEMORY;
        }
    }

    lp->maxSize = maxSize;

    return 0;
}


/*
 *  Add an item to the list and return the item index.
 */
int ejsAddItem(MprCtx ctx, EjsList *lp, cvoid *item)
{
    int     index, capacity;

    mprAssert(lp);
    mprAssert(lp->length >= 0);

    capacity = CAPACITY(lp->items);
    mprAssert(capacity >= 0);

    if (lp->items == 0 || lp->length >= capacity) {
        if (growList(ctx, lp, 1) < 0) {
            return MPR_ERR_TOO_MANY;
        }
    }

    index = lp->length++;
    lp->items[index] = (void*) item;

    return index;
}


int ejsAddItemToSharedList(MprCtx ctx, EjsList *lp, cvoid *item)
{
    EjsList     tmp;

    if (lp->items == NULL || mprGetParent(lp->items) != ctx) {
        tmp = *lp;
        lp->items = 0;
        lp->length = 0;
        if (ejsCopyList(ctx, lp, &tmp) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    return ejsAddItem(ctx, lp, item);
}


EjsList *ejsAppendList(MprCtx ctx, EjsList *list, EjsList *add)
{
    void        *item;
    int         next;

    mprAssert(list);
    mprAssert(list != add);

    for (next = 0; ((item = ejsGetNextItem(add, &next)) != 0); ) {
        if (ejsAddItem(ctx, list, item) < 0) {
            mprFree(list);
            return 0;
        }
    }
    return list;
}


void ejsClearList(EjsList *lp)
{
    int     i;

    mprAssert(lp);

    for (i = 0; i < lp->length; i++) {
        lp->items[i] = 0;
    }
    lp->length = 0;
}


int ejsCopyList(MprCtx ctx, EjsList *dest, EjsList *src)
{
    void        *item;
    int         next, capacity;

    ejsClearList(dest);

    capacity = CAPACITY(src->items);
    if (ejsSetListLimits(ctx, dest, capacity, src->maxSize) < 0) {
        return MPR_ERR_NO_MEMORY;
    }

    for (next = 0; (item = ejsGetNextItem(src, &next)) != 0; ) {
        if (ejsAddItem(ctx, dest, item) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    return 0;
}


void *ejsGetItem(EjsList *lp, int index)
{
    mprAssert(lp);

    if (index < 0 || index >= lp->length) {
        return 0;
    }
    return lp->items[index];
}


void *ejsGetLastItem(EjsList *lp)
{
    mprAssert(lp);

    if (lp == 0) {
        return 0;
    }

    if (lp->length == 0) {
        return 0;
    }
    return lp->items[lp->length - 1];
}


void *ejsGetNextItem(EjsList *lp, int *next)
{
    void    *item;
    int     index;

    mprAssert(next);
    mprAssert(*next >= 0);

    if (lp == 0) {
        return 0;
    }

    index = *next;

    if (index < lp->length) {
        item = lp->items[index];
        *next = ++index;
        return item;
    }
    return 0;
}


int ejsGetListCount(EjsList *lp)
{
    if (lp == 0) {
        return 0;
    }

    return lp->length;
}


void *ejsGetPrevItem(EjsList *lp, int *next)
{
    int     index;

    mprAssert(next);

    if (lp == 0) {
        return 0;
    }

    if (*next < 0) {
        *next = lp->length;
    }
    index = *next;

    if (--index < lp->length && index >= 0) {
        *next = index;
        return lp->items[index];
    }
    return 0;
}


int ejsRemoveLastItem(EjsList *lp)
{
    mprAssert(lp);
    mprAssert(lp->length > 0);

    if (lp->length <= 0) {
        return MPR_ERR_NOT_FOUND;
    }
    return ejsRemoveItemAtPos(lp, lp->length - 1);
}


/*
 *  Remove an index from the list. Return the index where the item resided.
 */
int ejsRemoveItemAtPos(EjsList *lp, int index)
{
    void    **items;
    int     i;

    mprAssert(lp);
    mprAssert(index >= 0);
    mprAssert(lp->length > 0);

    if (index < 0 || index >= lp->length) {
        return MPR_ERR_NOT_FOUND;
    }

    items = lp->items;
    for (i = index; i < (lp->length - 1); i++) {
        items[i] = items[i + 1];
    }
    lp->length--;
    lp->items[lp->length] = 0;

    return index;
}


/*
 *  Grow the list by the requried increment
 */
static int growList(MprCtx ctx, EjsList *lp, int incr)
{
    int     len, memsize, capacity;

    /*
     *  Need to grow the list
     */
    capacity = CAPACITY(lp->items);
    mprAssert(capacity >= 0);
    
    if (capacity >= lp->maxSize) {
        if (lp->maxSize == 0) {
            lp->maxSize = INT_MAX;
        } else {
            mprAssert(capacity < lp->maxSize);
            return MPR_ERR_TOO_MANY;
        }
    }

    /*
     *  If growing by 1, then use the default increment which exponentially grows.
     *  Otherwise, assume the caller knows exactly how the list needs to grow.
     */
    if (incr <= 1) {
        len = MPR_LIST_INCR + capacity + capacity;
    } else {
        len = capacity + incr;
    }
    memsize = len * sizeof(void*);

    /*
     *  Grow the list of items. Use the existing context for lp->items if it already exists. Otherwise use the list as the
     *  memory context owner.
     */
    mprAssert(memsize < (1024 * 1024));  //TODO sanity
    lp->items = (void**) mprRealloc(ctx, lp->items, memsize);

    /*
     *  Zero the new portion (required for no-compact lists)
     */
    memset(&lp->items[capacity], 0, sizeof(void*) * (len - capacity));

    return 0;
}


#if UNUSED
/*
 *  Change the item in the list at index. Return the old item.
 */
void *ejsSetItem(MprCtx ctx, EjsList *lp, int index, cvoid *item)
{
    void    *old;

    mprAssert(lp);
    mprAssert(lp->length >= 0);

    if (index >= lp->length) {
        lp->length = index + 1;
    }
    capacity = CAPACITY(lp->items);
    if (lp->length > capacity) {
        if (growList(ctx, lp, lp->length - capacity) < 0) {
            return 0;
        }
    }

    old = lp->items[index];
    lp->items[index] = (void*) item;

    return old;
}


/*
 *  Insert an item to the list at a specified position. We insert before "index".
 */
int ejsInsertItemAtPos(MprCtx ctx, EjsList *lp, int index, cvoid *item)
{
    void    **items;
    int     i;

    mprAssert(lp);
    mprAssert(lp->length >= 0);

    if (lp->length >= CAPACITY(lp->items)) {
        if (growList(ctx, lp, 1) < 0) {
            return MPR_ERR_TOO_MANY;
        }
    }

    /*
     *  Copy up items to make room to insert
     */
    items = lp->items;
    for (i = lp->length; i > index; i--) {
        items[i] = items[i - 1];
    }

    lp->items[index] = (void*) item;
    lp->length++;

    return index;
}


/*
 *  Remove an item from the list. Return the index where the item resided.
 */
int ejsRemoveItem(MprCtx ctx, EjsList *lp, void *item)
{
    int     index;

    mprAssert(lp);
    mprAssert(lp->length > 0);

    index = ejsLookupItem(lp, item);
    if (index < 0) {
        return index;
    }

    return ejsRemoveItemAtPos(ctx, lp, index);
}


/*
 *  Remove a set of items. Return 0 if successful.
 */
int ejsRemoveRangeOfItems(EjsList *lp, int start, int end)
{
    void    **items;
    int     i, count, capacity;

    mprAssert(lp);
    mprAssert(lp->length > 0);
    mprAssert(start > end);

    if (start < 0 || start >= lp->length) {
        return MPR_ERR_NOT_FOUND;
    }
    if (end < 0 || end >= lp->length) {
        return MPR_ERR_NOT_FOUND;
    }
    if (start > end) {
        return MPR_ERR_BAD_ARGS;
    }

    /*
     *  Copy down to coejsess
     */
    items = lp->items;
    count = end - start;
    for (i = start; i < (lp->length - count); i++) {
        items[i] = items[i + count];
    }
    lp->length -= count;
    capacity = CAPACITY(lp->items);
    for (i = lp->length; i < capacity; i++) {
        items[i] = 0;
    }

    return 0;
}


void *ejsGetFirstItem(EjsList *lp)
{
    mprAssert(lp);

    if (lp == 0) {
        return 0;
    }

    if (lp->length == 0) {
        return 0;
    }
    return lp->items[0];
}


int ejsGetListCapacity(EjsList *lp)
{
    mprAssert(lp);

    if (lp == 0) {
        return 0;
    }

    return CAPACITY(lp->items);
}


int ejsLookupItem(EjsList *lp, cvoid *item)
{
    int     i;

    mprAssert(lp);
    
    for (i = 0; i < lp->length; i++) {
        if (lp->items[i] == item) {
            return i;
        }
    }
    return MPR_ERR_NOT_FOUND;
}
#endif


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsList.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsLoader.c"
 */
/************************************************************************/

/**
 *  ejsLoader.c - Ejscript module file file loader
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

//  TODO - refactor entire loader




static int  addFixup(Ejs *ejs, int kind, EjsVar *target, int slotNum, EjsTypeFixup *fixup);
static EjsTypeFixup *createFixup(Ejs *ejs, EjsName *qname, int slotNum);
static int  fixupTypes(Ejs *ejs);
static int  loadBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadClassSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadDependencySection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndClassSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndModuleSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadExceptionSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static EjsModule *loadModuleSection(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, int *created, int flags);
static int  loadSections(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, MprList *modules, int flags);
static int  loadPropertySection(Ejs *ejs, MprFile *file, EjsModule *mp, int sectionType);
static int  loadScriptModule(Ejs *ejs, MprFile *file, cchar *path, MprList *modules, int flags);
static int  readNumber(Ejs *ejs, MprFile *file, int *number);
static char *tokenToString(EjsModule *mp, int   token);

#if !BLD_FEATURE_STATIC
static int  loadNativeLibrary(Ejs *ejs, cchar *name, cchar *path);
#endif

#if BLD_FEATURE_EJS_DOC
static int  loadDocSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static void setDoc(Ejs *ejs, EjsModule *mp, EjsVar *block, int slotNum);
#endif

/**
 *  Load a module file and return a list of the loaded modules. This is used to load scripted module files with
 *  optional native (shared / DLL) implementations. If loading a scripted module that has native declarations, a
 *  search for the corresponding native DLL will be performed and both scripted and native module files will be loaded.
 *  NOTE: this may recursively call itself as it loads dependent modules.
 *
 *  @param ejs Ejs handle
 *  @param nameArg Module name to load. May be "." separated path. May include or omit the ".mod" extension.
 *  @param url Optional URL to locate the module. Currently ignored
 *  @param callback Callback notification on load events.
 *  @param flags Reserved. Must be set to zero.
 *  @return Returns the last loaded module.
 *
 *  TODO - refactor. cleanup error handling
 */
MprList *ejsLoadModule(Ejs *ejs, cchar *nameArg, cchar *url, EjsLoaderCallback callback, int flags)
{
    MprFile             *file;
    MprList             *modules;
    EjsNativeCallback   moduleCallback;
    EjsModule           *mp;
    char                *cp, *path, cwd[MPR_MAX_FNAME], dir[MPR_MAX_FNAME], name[MPR_MAX_FNAME], filename[MPR_MAX_FNAME];
    int                 rc, alreadyLoading, next;

    mprAssert(nameArg && *nameArg);
    path = 0;

    /*
     *  Add .mod extension
     */
    mprStrcpy(name, sizeof(name), nameArg);
    if ((cp = strrchr(name, '.')) != 0 && strcmp(cp, EJS_MODULE_EXT) == 0) {
        *cp = '\0';
    }
    mprSprintf(filename, sizeof(filename), "%s%s", name, EJS_MODULE_EXT);

    /*
     *  TODO should distinguish between module names and module names?  A module may not match the name of any module.
     */
    mp = ejsLookupModule(ejs, name);
    if (mp && mp->loaded) {
        return mprCreateList(ejs);
    }

    mp = 0;
    ejs->loaderCallback = callback;

    /* TODO - Refactor need api for ejs->flags */
    alreadyLoading = ejs->flags & EJS_FLAG_LOADING;
    ejs->flags |= EJS_FLAG_LOADING;
    if (!alreadyLoading) {
        //  TODO - not a great place to store this
        if ((ejs->typeFixups = mprCreateList(ejs)) == 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }

    if (ejsSearch(ejs, &path, filename) < 0) {
        mprLog(ejs, 2, "Can't find module file \"%s.mod\" in search path \"%s\"", name,
            ejs->service->ejsPath ? ejs->service->ejsPath : "");
        ejsThrowReferenceError(ejs,  "Can't find module file \"%s", filename);
        return 0;
    }
    mprAssert(path);

    getcwd(cwd, sizeof(cwd));
    mprLog(ejs, 3, "Loading module %s, cwd %s", path, cwd);

    if ((file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0666)) == 0) {
        ejsThrowIOError(ejs, "Can't open module file %s", path);
        mprFree(path);
        return 0;
    }
    mprEnableFileBuffering(file, 0, 0);

    modules = mprCreateList(ejs);
    if (modules == 0) {
        mprFree(file);
        mprFree(path);
        ejsThrowMemoryError(ejs);
        return 0;
    }

    if (loadScriptModule(ejs, file, path, modules, flags) < 0) {
        mprFree(file);
        mprFree(modules);
        mprFree(path);
        return 0;
    }
    mprGetDirName(dir, sizeof(dir), path);

    /*
     *  Fixup type references across all modules. This solves the forward type reference problem.
     */
    if (! alreadyLoading) {
        ejs->flags &= ~EJS_FLAG_LOADING;
        if ((rc = fixupTypes(ejs)) < 0) {
            mprFree(file);
            mprFree(modules);
            mprFree(path);
            return 0;
        }
        mprFree(ejs->typeFixups);
        ejs->typeFixups = 0;
    }

    for (next = 0; (mp = mprGetNextItem(modules, &next)) != 0; ) {

        if (mp->hasNative && !mp->configured) {
            /*
             *  See if a native module initialization routine has been registered. If so, use that. Otherwise, look
             *  for a backing DSO.
             */
            if ((moduleCallback = (EjsNativeCallback) mprLookupHash(ejs->nativeModules, mp->name)) != 0) {
                if ((moduleCallback)(ejs, mp, path) < 0) {
                    if (ejs->exception == 0) {
                        ejsThrowIOError(ejs, "Can't load the native module file \"%s\"", path);
                    }
                    mprFree(file);
                    mprFree(modules);
                    mprFree(path);
                    return 0;
                }
                
            } else {
#if !BLD_FEATURE_STATIC
                rc = loadNativeLibrary(ejs, mp->name, dir);
                if (rc < 0) {
                    if (ejs->exception == 0) {
                        ejsThrowIOError(ejs, "Can't load the native module file \"%s\"", path);
                    }
                    mprFree(file);
                    mprFree(modules);
                    mprFree(path);
                    return 0;
                }
#endif
            }
        }

        mp->configured = 1;
            
        if (!(ejs->flags & EJS_FLAG_NO_EXE)) {
            if (ejsRunInitializer(ejs, mp) == 0) {
                mprFree(modules);
                mprFree(file);
                mprFree(path);
                return 0;
            }
        }
    }

    mprFree(file);
    mprFree(path);

    return modules;
}


/*
 *  Load the sections: classes, properties and functions. Return the first module loaded in pup.
 */
static int loadSections(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, MprList *modules, int flags)
{
    EjsModule   *mp, *firstModule;
    int         rc, sectionType, created, oldGen;

    created = 0;

    firstModule = mp = 0;

    /*
     *  For now, all loaded types are eternal.
     *  TODO - OPT. should relax this.
     */
    oldGen = ejsSetGeneration(ejs, EJS_GEN_ETERNAL);

    while ((sectionType = mprGetc(file)) >= 0) {

        if (sectionType < 0 || sectionType >= EJS_SECT_MAX) {
            //  TODO
            mprError(ejs, "Bad section type %d in %s", sectionType, mp->name);
            ejsSetGeneration(ejs, oldGen);
            return EJS_ERR;
        }
        mprLog(ejs, 7, "Load section type %d", sectionType);

        rc = 0;
        switch (sectionType) {

        case EJS_SECT_BLOCK:
            rc = loadBlockSection(ejs, file, mp);
            break;

        case EJS_SECT_BLOCK_END:
            rc = loadEndBlockSection(ejs, file, mp);
            break;

        case EJS_SECT_CLASS:
            rc = loadClassSection(ejs, file, mp);
            break;

        case EJS_SECT_CLASS_END:
            rc = loadEndClassSection(ejs, file, mp);
            break;

        case EJS_SECT_DEPENDENCY:
            rc = loadDependencySection(ejs, file, mp);
            break;

        case EJS_SECT_EXCEPTION:
            rc = loadExceptionSection(ejs, file, mp);
            break;

        case EJS_SECT_FUNCTION:
            rc = loadFunctionSection(ejs, file, mp);
            break;

        case EJS_SECT_FUNCTION_END:
            rc = loadEndFunctionSection(ejs, file, mp);
            break;

        case EJS_SECT_MODULE:
            mp = loadModuleSection(ejs, file, hdr, &created, flags);
            if (mp == 0) {
                ejsSetGeneration(ejs, oldGen);
                return 0;
            }
            if (firstModule == 0) {
                firstModule = mp;
            }
            ejsAddModule(ejs, mp);
            mprAddItem(modules, mp);
            break;

        case EJS_SECT_MODULE_END:
            rc = loadEndModuleSection(ejs, file, mp);
            break;

        case EJS_SECT_PROPERTY:
            rc = loadPropertySection(ejs, file, mp, sectionType);
            break;

#if BLD_FEATURE_EJS_DOC
        case EJS_SECT_DOC:
            rc = loadDocSection(ejs, file, mp);
            break;
#endif

        default:
            mprAssert(0);
            ejsSetGeneration(ejs, oldGen);
            return EJS_ERR;
        }

        if (rc < 0) {
            if (mp && mp->name && created) {
                ejsRemoveModule(ejs, mp);
                mprRemoveItem(modules, mp);
                mprFree(mp);
            }
            ejsSetGeneration(ejs, oldGen);
            return rc;
        }
    }

    ejsSetGeneration(ejs, oldGen);
    return 0;
}


/*
 *  Load a module section and constant pool.
 */
static EjsModule *loadModuleSection(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, int *created, int flags)
{
    EjsModule   *mp;
    char        *pool, *name, *url;
    int         rc, poolSize, nameToken, urlToken;

    mprAssert(created);

    *created = 0;

    /*
     *  We don't have the constant pool yet so we cant resolve the name yet.
     */
    if (readNumber(ejs, file, &nameToken) < 0) {
        return 0;
    }
    if (readNumber(ejs, file, &urlToken) < 0) {
        return 0;
    }

    if (readNumber(ejs, file, &poolSize) < 0) {
        //  TODO DIAG on all returns
        return 0;
    }
    if (poolSize <= 0 || poolSize > EJS_MAX_POOL) {
        return 0;
    }

    /*
     *  Read the string constant pool
     */
    pool = (char*) mprAlloc(file, poolSize);
    if (pool == 0) {
        return 0;
    }
    if (mprRead(file, pool, poolSize) != poolSize) {
        mprFree(pool);
        return 0;
    }

    /*
     *  Convert module token into a name
     */
    //  TODO - need an API for this
    if (nameToken < 0 || nameToken >= poolSize) {
        //  TODO DIAG
        mprAssert(0);
        return 0;
    }
    name = &pool[nameToken];
    if (name == 0) {
        mprAssert(name);
        mprFree(pool);
        return 0;
    }
    if (urlToken < 0 || urlToken >= poolSize) {
        //  TODO DIAG
        mprAssert(0);
        return 0;
    }
    url = &pool[urlToken];
    if (url == 0) {
        mprAssert(url);
        mprFree(pool);
        return 0;
    }

    /*
     *  Check if the module is already loaded
     */
    rc = ejsCheckModuleLoaded(ejs, name);
    if (rc < 0) {
        mprFree(pool);
        return 0;

    } else if (rc == 1) {
        mprFree(pool);
        return ejsLookupModule(ejs, name);
    }

    mp = ejsCreateModule(ejs, name, url, pool, poolSize);
    if (mp == 0) {
        mprFree(pool);
        return 0;
    }
    *created = 1;

    if (strcmp(name, EJS_DEFAULT_MODULE) != 0) {
        /*
         *  Signify that loading the module has begun. We allow multiple loads into the default module.
         */
        mp->loaded = 1;
        mp->constants->locked = 1;
    }
    if (hdr->flags & EJS_MODULE_BOUND_GLOBALS) {
        mp->boundGlobals = 1;
    }

    mp->file = file;
    mp->flags = flags;
#if UNUSED
    mp->seq = hdr->seq;
#endif

    mp->firstGlobalSlot = ejsGetPropertyCount(ejs, ejs->global);

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_MODULE, mp);
    }

    mprLog(ejs, 6, "Load module section %s", name);

    return mp;
}


static int loadEndModuleSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    mprLog(ejs, 7, "End module section %s", mp->name);

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_MODULE_END, mp);
    }

    return 0;
}


static int loadDependencySection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsModule   *module;
    MprList     *modules;
    void        *callback;
    char        *name, *url;
    int         next;

    mprAssert(ejs);
    mprAssert(file);
    mprAssert(mp);

    name = ejsModuleReadString(ejs, mp);
    url = ejsModuleReadString(ejs, mp);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }
    
    mprLog(ejs, 7, "    Load dependency section %s", name);

    callback = ejs->loaderCallback;
    modules = ejsLoadModule(ejs, name, url, NULL, mp->flags);
    ejs->loaderCallback = callback;

    if (modules == 0) {
        return MPR_ERR_CANT_READ;
    }

    if (mp->dependencies == 0) {
        mp->dependencies = mprCreateList(mp);
    }

    for (next = 0; (module = mprGetNextItem(modules, &next)) != 0; ) {
        mprAddItem(mp->dependencies, module);

        if (ejs->loaderCallback) {
            (ejs->loaderCallback)(ejs, EJS_SECT_DEPENDENCY, module, name, url);
        }
    }
    return 0;
}


static int loadBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsBlock    *block;
    EjsVar      *owner;
    EjsName     qname;
    int         slotNum, numSlot;

    qname.space = EJS_BLOCK_NAMESPACE;
    qname.name = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadNumber(ejs, mp, &numSlot);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }
    
    block = ejsCreateBlock(ejs, qname.name, numSlot);
    owner = (EjsVar*) mp->scopeChain;

    if (ejsLookupProperty(ejs, owner, &qname) >= 0) {
        ejsThrowReferenceError(ejs, "Block \"%s\" already loaded", qname.name);
        return MPR_ERR_CANT_CREATE;
    }

    if (owner == ejs->global && !mp->boundGlobals) {
        slotNum = -1;
    }
    slotNum = ejsDefineProperty(ejs, owner, slotNum, &qname, ejs->blockType, 0, (EjsVar*) block);
    if (slotNum < 0) {
        return MPR_ERR_CANT_WRITE;
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_BLOCK, mp, owner, slotNum, qname.name, numSlot, block);
    }

    block->scopeChain = mp->scopeChain;
    mp->scopeChain = block;

    return 0;
}


static int loadEndBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    mprLog(ejs, 7, "    End block section %s", mp->name);

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_BLOCK_END, mp);
    }

    mp->scopeChain = mp->scopeChain->scopeChain;

    return 0;
}


static int loadClassSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsType         *type, *baseType, *iface, *nativeType;
    EjsTypeFixup    *fixup, *ifixup;
    EjsName         qname, baseClassName, ifaceClassName;
    EjsBlock        *block;
    int             attributes, numTypeProp, numInstanceProp, slotNum, numInterfaces, i;

    fixup = 0;
    ifixup = 0;
    
    qname.name = ejsModuleReadString(ejs, mp);
    qname.space = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &attributes);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadType(ejs, mp, &baseType, &fixup, &baseClassName, 0);
    ejsModuleReadNumber(ejs, mp, &numTypeProp);
    ejsModuleReadNumber(ejs, mp, &numInstanceProp);
    ejsModuleReadNumber(ejs, mp, &numInterfaces);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }
    if (ejsLookupProperty(ejs, ejs->global, &qname) >= 0) {
        ejsThrowReferenceError(ejs, "Class \"%s\" already loaded", qname.name);
        return MPR_ERR_CANT_CREATE;
    }
    if (fixup || (baseType && baseType->needFixup)) {
        attributes |= EJS_ATTR_SLOTS_NEED_FIXUP;
    }

    /*
     *  Find pre-existing native types.
     */
    if (attributes & EJS_ATTR_NATIVE) {
        type = nativeType = (EjsType*) mprLookupHash(ejs->coreTypes, qname.name);
        if (type == 0) {
            mprLog(ejs, 1, "WARNING: can't find native type \"%s\"", qname.name);
        }
    } else {
        type = nativeType = 0;
#if BLD_DEBUG
        if (mprLookupHash(ejs->coreTypes, qname.name)) {
            mprError(ejs, "WARNING: type \"%s\" defined as a native type but not declared as native", qname.name);
        }
#endif
    }

    if (mp->flags & EJS_MODULE_BUILTIN) {
        attributes |= EJS_ATTR_BUILTIN;
    }
    if (attributes & EJS_ATTR_SLOTS_NEED_FIXUP) {
        baseType = 0;
        if (fixup == 0) {
            // TODO - was &qname, slotNum
            fixup = createFixup(ejs, (baseType) ? &baseType->qname : &ejs->objectType->qname, -1);
        }
    }
    
    mprAssert(slotNum >= 0);
    
    /*
     *  If the module is fully bound (--merge), then we install the type at the prescribed slot number.
     */
    if (! mp->boundGlobals) {
        slotNum = -1;
    }
    if (slotNum < 0) {
        slotNum = ejs->globalBlock->obj.numProp;
    }
    
    if (type == 0) {
        attributes |= EJS_ATTR_OBJECT | EJS_ATTR_OBJECT_HELPERS;
        type = ejsCreateType(ejs, &qname, mp, baseType, sizeof(EjsObject), slotNum, numTypeProp, numInstanceProp, 
            attributes, 0);
        if (type == 0) {
            ejsThrowInternalError(ejs, "Can't create class %s", qname.name);
            return MPR_ERR_BAD_STATE;
        }

    } else {
        mp->hasNative = 1;
        if (attributes & EJS_ATTR_HAS_CONSTRUCTOR && !type->hasConstructor) {
            mprError(ejs, "WARNING: module indicates a constructor required but none exists for \"%s\"", type->qname.name);
        }
        if (!type->block.obj.var.native) {
            mprError(ejs, "WARNING: type not defined as native: \"%s\"", type->qname.name);
        }
    }
    
    /*
     *  Read implemented interfaces. Add to type->implements. Create fixup record if the interface type is not yet known.
     */
    if (numInterfaces > 0) {
        type->implements = mprCreateList(type);
        for (i = 0; i < numInterfaces; i++) {
            if (ejsModuleReadType(ejs, mp, &iface, &ifixup, &ifaceClassName, 0) < 0) {
                return MPR_ERR_CANT_READ;
            }
            if (iface) {
                mprAddItem(type->implements, iface);
            } else {
                if (addFixup(ejs, EJS_FIXUP_INTERFACE_TYPE, (EjsVar*) type, -1, ifixup) < 0) {
                    ejsThrowMemoryError(ejs);
                    return MPR_ERR_NO_MEMORY;
                }
            }
        }
    }

    if (mp->flags & EJS_MODULE_BUILTIN) {
        type->block.obj.var.builtin = 1;
    }
    if (attributes & EJS_ATTR_HAS_STATIC_INITIALIZER) {
        type->hasStaticInitializer = 1;
    }
    if (attributes & EJS_ATTR_DYNAMIC_INSTANCE) {
        type->dynamicInstance = 1;
    }

    mprLog(ejs, 6, "    Load %s class %s for module %s at slot %d", qname.space, qname.name, mp->name, slotNum);

    slotNum = ejsDefineProperty(ejs, ejs->global, slotNum, &qname, ejs->typeType, attributes, (EjsVar*) type);
    if (slotNum < 0) {
        ejsThrowMemoryError(ejs);
        return MPR_ERR_NO_MEMORY;
    }
    type->module = mp;

    if (fixup) {
        if (addFixup(ejs, EJS_FIXUP_BASE_TYPE, (EjsVar*) type, -1, fixup) < 0) {
            ejsThrowMemoryError(ejs);
            return MPR_ERR_NO_MEMORY;
        }
        
    } else {
        if (ejs->flags & EJS_FLAG_EMPTY) {
            if (attributes & EJS_ATTR_NATIVE) {
                /*
                 *  When empty, native types are created with no properties and with numTraits equal to zero. 
                 *  This is so the compiler can compile the core ejs module. For ejsmod which may also run in 
                 *  empty mode, we set numInherited here to the correct value for native types.
                 */
                if (type->block.numInherited == 0 && type->baseType) {
                    type->block.numInherited = type->baseType->block.numTraits;
                }
            }

#if FUTURE
        } else {
            if (nativeType && !type->isInterface) {
                /*
                 *  Inherit native methods. Need to clone the function if it is a native function
                 */
                mprAssert(baseType);
                for (i = 0; i < type->block.obj.numProp; i++) {
                    existingFun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, i);
                    if (ejsIsFunction(existingFun)) {
                        fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) baseType, i);
                        if (existingFun->override) {
                            continue;
                        }
                        if (ejsIsNativeFunction(fun)) {
                            fun = (EjsFunction*) ejsCloneVar(ejs, fun);
                        }
                        ejsSetProperty(ejs, (EjsVar*) type, i, (EjsVar*) fun);
                    }
                }
            }
#endif
        }
    }

#if BLD_FEATURE_EJS_DOC
    setDoc(ejs, mp, ejs->global, slotNum);
#endif

    block = (EjsBlock*) type;
    block->scopeChain = mp->scopeChain;
    mp->scopeChain = block;

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_CLASS, mp, slotNum, qname, type, attributes);
    }

    return 0;
}


static int loadEndClassSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsType     *type;

    mprLog(ejs, 7, "    End class section");

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_CLASS_END, mp, mp->scopeChain);
    }

    type = (EjsType*) mp->scopeChain;
    if (type->block.hasScriptFunctions && type->baseType) {
        ejsDefineTypeNamespaces(ejs, type);
    }
    mp->scopeChain = mp->scopeChain->scopeChain;

    return 0;
}


//  TODO - break into functions.

static int loadFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsType         *returnType;
    EjsTypeFixup    *fixup;
    EjsFunction     *fun;
    EjsName         qname, returnTypeName;
    EjsBlock        *block;
    uchar           *code;
    int             slotNum, numArgs, codeLen, numLocals, numExceptions, attributes, nextSlot, lang;

    lang = 0;

    qname.name = ejsModuleReadString(ejs, mp);
    qname.space = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &nextSlot);
    ejsModuleReadNumber(ejs, mp, &attributes);
    ejsModuleReadByte(ejs, mp, &lang);
 
    ejsModuleReadType(ejs, mp, &returnType, &fixup, &returnTypeName, 0);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadNumber(ejs, mp, &numArgs);
    ejsModuleReadNumber(ejs, mp, &numLocals);
    ejsModuleReadNumber(ejs, mp, &numExceptions);
    ejsModuleReadNumber(ejs, mp, &codeLen);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }

    block = (EjsBlock*) mp->scopeChain;
    mprAssert(block);
    mprAssert(numArgs >= 0 && numArgs < EJS_MAX_ARGS);
    mprAssert(numLocals >= 0 && numLocals < EJS_MAX_LOCALS);
    mprAssert(numExceptions >= 0 && numExceptions < EJS_MAX_EXCEPTIONS);

    mprLog(ejs, 6, "Loading function %s:%s at slot %d", qname.space, qname.name, slotNum);

    /*
     *  Read the code. We don't need to store the code length as the verifier will ensure we always have returns 
     *  in all cases. ie. can't fall off the end. We pass ownership of the code to createMethod i.e. don't free.
     *  TODO - read the code after the function is created. That way won't need to steal the block.
     */
    if (codeLen > 0) {
        code = (uchar*) mprAlloc(ejs, codeLen);
        if (code == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        if (mprRead(file, code, codeLen) != codeLen) {
            mprFree(code);
            return MPR_ERR_CANT_READ;
        }
        block->hasScriptFunctions = 1;
        
    } else {
        code = 0;
    }

    if (attributes & EJS_ATTR_NATIVE) {
        mp->hasNative = 1;
    }
    if (attributes & EJS_ATTR_INITIALIZER) {
        mp->hasInitializer = 1;
    }
    if (mp->flags & EJS_MODULE_BUILTIN) {
        attributes |= EJS_ATTR_BUILTIN;
    }

    if (ejsLookupProperty(ejs, (EjsVar*) block, &qname) >= 0 && !(attributes & EJS_ATTR_OVERRIDE)) {
        if (ejsIsType(block)) {
            ejsThrowReferenceError(ejs,
                "function \"%s\" already defined in type \"%s\". Try adding \"override\" to the function declaration.", 
                qname.name, ((EjsType*) block)->qname.name);
        } else {
            ejsThrowReferenceError(ejs,
                "function \"%s\" already defined in block \"%s\". Try adding \"override\" to the function declaration.", 
                qname.name, block->name);
        }
        return MPR_ERR_CANT_CREATE;
    }

    /*
     *  Create the function using the current scope chain. Non-methods revise this scope chain via the 
     *  DefineFunction op code.
     */
    fun = ejsCreateFunction(ejs, code, codeLen, numArgs, numExceptions, returnType, attributes, mp->constants, 
        mp->scopeChain, lang);
    if (fun == 0) {
        mprFree(code);
        return MPR_ERR_NO_MEMORY;
    }
    mprStealBlock(fun, code);

    ejsSetDebugName(fun, qname.name);

    if (block == (EjsBlock*) ejs->global && !mp->boundGlobals) {
        /*
         *  Global property and not using --merge
         */
        if (attributes & EJS_ATTR_OVERRIDE) {
            //  TODO - namespace
            slotNum = ejsLookupProperty(ejs, (EjsVar*) block, &qname);
            if (slotNum < 0) {
                mprError(ejs, "Can't find method \"%s\" to override", qname.name);
                return MPR_ERR_NO_MEMORY;
            }

        } else {
            slotNum = -1;
        }
    }

    if (mp->flags & EJS_MODULE_BUILTIN) {
        fun->block.obj.var.builtin = 1;
    }

    if (attributes & EJS_ATTR_INITIALIZER && block == (EjsBlock*) ejs->global) {
        mp->initializer = fun;
        fun->isInitializer = 1;
        slotNum = -1;

    } else {
        slotNum = ejsDefineProperty(ejs, (EjsVar*) block, slotNum, &qname, ejs->functionType, attributes, (EjsVar*) fun);
        if (slotNum < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    
    ejsSetNextFunction(fun, nextSlot);

    if (fixup) {
        mprAssert(returnType == 0);
        if (addFixup(ejs, EJS_FIXUP_RETURN_TYPE, (EjsVar*) fun, -1, fixup) < 0) {
            ejsThrowMemoryError(ejs);
            return MPR_ERR_NO_MEMORY;
        }
    }

#if BLD_FEATURE_EJS_DOC
    setDoc(ejs, mp, (EjsVar*) block, slotNum);
#endif

    mp->currentMethod = fun;
    fun->block.scopeChain = mp->scopeChain;
    mp->scopeChain = &fun->block;

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_FUNCTION, mp, block, slotNum, qname, fun, attributes);
    }

    return 0;
}


static int loadEndFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsTrait            *trait;
    EjsFunction         *fun;
    int                 i;

    mprLog(ejs, 7, "    End function section");

    fun = (EjsFunction*) mp->scopeChain;

    for (i = 0; i < (int) fun->numArgs; i++) {
        trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, i);
        if (trait && trait->attributes & EJS_ATTR_INITIALIZER) {
            fun->numDefault++;
        }
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_FUNCTION_END, mp, fun);
    }

    mp->scopeChain = mp->scopeChain->scopeChain;

    return 0;
}


static int loadExceptionSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsFunction         *fun;
    EjsType             *catchType;
    EjsTypeFixup        *fixup;
    EjsCode             *code;
    EjsEx               *ex;
    int                 tryStart, tryEnd, handlerStart, handlerEnd;
    int                 flags, i;

    fun = mp->currentMethod;
    mprAssert(fun);

    flags = 0;
    code = &fun->body.code;

    for (i = 0; i < code->numHandlers; i++) {
        ejsModuleReadByte(ejs, mp, &flags);
        ejsModuleReadNumber(ejs, mp, &tryStart);
        ejsModuleReadNumber(ejs, mp, &tryEnd);
        ejsModuleReadNumber(ejs, mp, &handlerStart);
        ejsModuleReadNumber(ejs, mp, &handlerEnd);
        ejsModuleReadType(ejs, mp, &catchType, &fixup, 0, 0);

        if (mp->hasError) {
            return MPR_ERR_CANT_READ;
        }
    
        ex = ejsAddException(fun, tryStart, tryEnd, catchType, handlerStart, handlerEnd, flags, i);

        if (fixup) {
            mprAssert(catchType == 0);
            if (addFixup(ejs, EJS_FIXUP_EXCEPTION, (EjsVar*) ex, 0, fixup) < 0) {
                mprAssert(0);
                return MPR_ERR_NO_MEMORY;
            }
        }
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_EXCEPTION, mp, fun);
    }

    return 0;
}

int mystrcmp(char *s1, char *s2)
{
    return strcmp(s1, s2);
}

/*
 *  Define a global, class or block property. Not used for function locals or args.
 */
static int loadPropertySection(Ejs *ejs, MprFile *file, EjsModule *mp, int sectionType)
{
    EjsType         *type;
    EjsTypeFixup    *fixup;
    EjsName         qname, propTypeName;
    EjsVar          *block, *value;
    cchar           *str;
    int             slotNum, attributes, fixupKind;

    value = 0;
    block = (EjsVar*) mp->scopeChain;
    mprAssert(block);
    
    qname.name = ejsModuleReadString(ejs, mp);
    qname.space = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &attributes);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadType(ejs, mp, &type, &fixup, &propTypeName, 0);

    //  TODO - temporary and not ideal. Must encode name and uri. Should do this for all constants.
    if (attributes & EJS_ATTR_HAS_VALUE) {
        if ((str = ejsModuleReadString(ejs, mp)) == 0) {
            return MPR_ERR_CANT_READ;
        }
        /*  Only doing for namespaces currently */
        value = (EjsVar*) ejsCreateNamespace(ejs, str, str);
    }

    mprLog(ejs, 7, "Loading property %s:%s at slot %d", qname.space, qname.name, slotNum);

    if (attributes & EJS_ATTR_NATIVE) {
        mp->hasNative = 1;
    }
    if (mp->flags & EJS_MODULE_BUILTIN) {
        attributes |= EJS_ATTR_BUILTIN;
    }

    if (ejsLookupProperty(ejs, block, &qname) >= 0) {
        ejsThrowReferenceError(ejs, "property \"%s\" already loaded", qname.name);
        return MPR_ERR_CANT_CREATE;
    }

    if (ejsIsFunction(block)) {
        fixupKind = EJS_FIXUP_LOCAL;

    } else if (ejsIsType(block) && !(attributes & EJS_ATTR_STATIC) && block != ejs->global) {
        mprAssert(((EjsType*) block)->instanceBlock);
        block = (EjsVar*) ((EjsType*) block)->instanceBlock;
        fixupKind = EJS_FIXUP_INSTANCE_PROPERTY;

    } else {
        fixupKind = EJS_FIXUP_TYPE_PROPERTY;
    }

    if (block == ejs->global && !mp->boundGlobals) {
        slotNum = -1;
    }

    slotNum = ejsDefineProperty(ejs, block, slotNum, &qname, type, attributes, value);
    if (slotNum < 0) {
        return MPR_ERR_CANT_WRITE;
    }

    if (fixup) {
        mprAssert(type == 0);
        if (addFixup(ejs, fixupKind, block, slotNum, fixup) < 0) {
            ejsThrowMemoryError(ejs);
            return MPR_ERR_NO_MEMORY;
        }
    }

#if BLD_FEATURE_EJS_DOC
    setDoc(ejs, mp, block, slotNum);
#endif

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_PROPERTY, mp, block, slotNum, qname, attributes, propTypeName);
    }

    return 0;
}


#if BLD_FEATURE_EJS_DOC
static int loadDocSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    char        *doc;

    mprLog(ejs, 7, "    Documentation section");

    doc = ejsModuleReadString(ejs, mp);

    if (ejs->flags & EJS_FLAG_DOC) {
        mp->doc = doc;
        if (ejs->loaderCallback) {
            (ejs->loaderCallback)(ejs, EJS_SECT_DOC, doc);
        }
    }
    return 0;
}
#endif



#if !BLD_FEATURE_STATIC
#if UNUSED
static bool sharedLibrary(cchar *path)
{
    int     len, extLen;

    len = (int) strlen(path);
    extLen = (int) strlen(BLD_SHOBJ);

    if (len <= extLen || mprStrcmpAnyCase(&path[len - extLen], BLD_SHOBJ) != 0) {
        return 0;
    }
    return 1;
}
#endif


/*
 *  Check if a native module exists at the given path. If so, load it. If the path is a scripted module
 *  but has a corresponding native module, then load that. Return 1 if loaded, -1 for errors, 0 if no
 *  native module found.
 */
static int loadNativeLibrary(Ejs *ejs, cchar *name, cchar *dir)
{
    char    path[MPR_MAX_PATH], initName[MPR_MAX_PATH], moduleName[MPR_MAX_PATH], *cp;

    if (ejs->flags & EJS_FLAG_NO_EXE) {
        return 0;
    }

    mprSprintf(path, sizeof(path), "%s/%s%s", dir, name, BLD_SHOBJ);
    if (! mprAccess(ejs, path, R_OK)) {
        return 0;
    }

    /*
     *  Build the DSO entry point name. Format is "NameModuleInit" where Name has "." converted to "_"
     */
    mprStrcpy(moduleName, sizeof(moduleName), name);
    moduleName[0] = tolower(moduleName[0]);
    mprSprintf(initName, sizeof(initName), "%sModuleInit", moduleName);
    for (cp = initName; *cp; cp++) {
        if (*cp == '.') {
            *cp = '_';
        }
    }

    if (mprLookupModule(ejs, name) != 0) {
        mprLog(ejs, 1, "Native module \"%s\" is already loaded", path);
        return 0;
    }

    if (mprLoadModule(ejs, path, initName) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    return 1;
}
#endif


/*
 *  Load a scripted module file. Return a modified list of modules.
 */
static int loadScriptModule(Ejs *ejs, MprFile *file, cchar *path, MprList *modules, int flags)
{
    EjsModuleHdr    hdr;

    mprAssert(path);

    /*
     *  Read module file header
     *  TODO - module header byte order
     */
    if ((mprRead(file, &hdr, sizeof(hdr))) != sizeof(hdr)) {
        //  TODO - should not throw exceptions in this file. It is used by the compiler.
        ejsThrowIOError(ejs, "Error reading module file %s, corrupt header", path);
        return EJS_ERR;
    }

    if ((int) hdr.magic != EJS_MODULE_MAGIC) {
        ejsThrowIOError(ejs, "Bad module file format in %s", path);
        return EJS_ERR;
    }
    if ((int) hdr.major != EJS_MAJOR || (int) hdr.minor != EJS_MINOR) {
        ejsThrowIOError(ejs, "Incompatible module file format in %s", path);
        return EJS_ERR;
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_START, path, &hdr);
    }

    /*
     *  Load the sections: classes, properties and functions.
     *  NOTE: this may load multiple modules.
     */
    if (loadSections(ejs, file, &hdr, modules, flags) < 0) {
        if (ejs->exception == 0) {
            ejsThrowReferenceError(ejs, "Can't load module file %s", path);
        }
        return EJS_ERR;
    }
    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_END, modules, 0);
    }

    return 0;
}


static int fixupTypes(Ejs *ejs)
{
    MprList         *list;
    EjsTypeFixup    *fixup;
    EjsModule       *mp;
    EjsType         *type, *targetType;
    EjsBlock        *instanceBlock;
    EjsTrait        *trait;
    EjsFunction     *targetFunction;
    EjsEx           *targetException;
    int             next;

    list = ejs->typeFixups;

    /*
        TODO - this could be optimized. We often are looking up the same property.
        When creating fixups, if the type is the same as the previous fixup, we could point to the previous entry.
     */
    for (next = 0; (fixup = (EjsTypeFixup*) mprGetNextItem(list, &next)) != 0; ) {

        mp = 0;
        type = 0;

        if (fixup->typeSlotNum >= 0) {
            type = (EjsType*) ejsGetProperty(ejs, ejs->global, fixup->typeSlotNum);

        } else if (fixup->typeName.name) {
            mprAssert(fixup->typeSlotNum < 0);
            type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &fixup->typeName);
            
        } else {
            continue;
        }
        if (type == 0) {
            ejsThrowReferenceError(ejs, "Can't fixup forward type reference for %s. Fixup kind %d", 
                fixup->typeName.name, fixup->kind);
            mprError(ejs, "Can't fixup forward type reference for %s. Fixup kind %d", fixup->typeName.name, fixup->kind);
            return EJS_ERR;
        }

        switch (fixup->kind) {
        case EJS_FIXUP_BASE_TYPE:
            mprAssert(fixup->target);
            targetType = (EjsType*) fixup->target;
            targetType->needFixup = 1;
            ejsFixupClass(ejs, targetType, type, targetType->implements, 0);
            instanceBlock = targetType->instanceBlock;
            if (instanceBlock && type) {
                ejsFixupBlock(ejs, instanceBlock, type->instanceBlock, targetType->implements, 0);
            }
            if (targetType->block.namespaces.length == 0 && type->block.hasScriptFunctions) {
                ejsDefineTypeNamespaces(ejs, targetType);
            }
            break;

        case EJS_FIXUP_INTERFACE_TYPE:
            targetType = (EjsType*) fixup->target;
            mprAddItem(targetType->implements, type);
            break;

        case EJS_FIXUP_RETURN_TYPE:
            mprAssert(fixup->target);
            targetFunction = (EjsFunction*) fixup->target;
            targetFunction->resultType = type;
            break;

        case EJS_FIXUP_TYPE_PROPERTY:
            mprAssert(fixup->target);
            trait = ejsGetPropertyTrait(ejs, fixup->target, fixup->slotNum);
            mprAssert(trait);
            if (trait) {
                trait->type = type;
            }
            break;

        case EJS_FIXUP_INSTANCE_PROPERTY:
            mprAssert(fixup->target);
            mprAssert(ejsIsBlock(fixup->target));
            mprAssert(fixup->target->isInstanceBlock);
            trait = ejsGetPropertyTrait(ejs, fixup->target, fixup->slotNum);
            mprAssert(trait);
            if (trait) {
                trait->type = type;
            }
            break;

        case EJS_FIXUP_LOCAL:
            mprAssert(fixup->target);
            trait = ejsGetPropertyTrait(ejs, fixup->target, fixup->slotNum);
            mprAssert(trait);
            if (trait) {
                trait->type = type;
            }
            break;

        case EJS_FIXUP_EXCEPTION:
            mprAssert(fixup->target);
            targetException = (EjsEx*) fixup->target;
            targetException->catchType = type;
            break;

        default:
            mprAssert(0);
        }
    }
    return 0;
}


/*
 *  Search for a file. If found, Return the path where the file was located. Otherwise return null.
 */
static char *probe(Ejs *ejs, cchar *path)
{
    mprAssert(ejs);
    mprAssert(path);

    mprLog(ejs, 9, "Probe for file %s", path);
    if (mprAccess(ejs, path, R_OK)) {
        return mprStrdup(ejs, path);
    }
    return 0;
}


/*
 *  Search for a file. Return the full path name of the file.
 *  The search strategy is: Given a name "a.b.c", scan for:
 *
 *      1. File named a.b.c
 *      2. File named a/b/c
 *      3. File named a.b.c in EJSPATH
 *      4. File named a/b/c in EJSPATH
 *      5. File named c in EJSPATH
 */
int ejsSearch(Ejs *ejs, char **path, cchar *name)
{
    cchar   *baseName, *ejsPath;
    char    fileName[MPR_MAX_FNAME];
    char    *searchPath, *dir, *tok, *cp, *slashName;

    slashName = 0;
    ejsPath = ejs->service->ejsPath;

    mprLog(ejs, 7, "Search for module \"%s\" in ejspath %s\n", name, ejsPath);

    /*
     *  1. Search for path directly
     */
    if ((*path = probe(ejs, name)) != 0) {
        mprLog(ejs, 7, "Found %s at %s", name, *path);
        return 0;
    }

    /*
     *  2. Search for "a/b/c"
     */
    slashName = mprStrdup(ejs, name);
    for (cp = slashName; *cp; cp++) {
        if (*cp == '.') {
            *cp = '/';
        }
    }
    if ((*path = probe(ejs, slashName)) != 0) {
        mprLog(ejs, 7, "Found %s at %s", name, *path);
        return 0;
    }

    /*
     *  3. Search for "a.b.c" in EJSPATH
     */
    searchPath = mprStrdup(ejs, ejsPath);
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, name);
        if ((*path = probe(ejs, fileName)) != 0) {
            mprLog(ejs, 7, "Found %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(searchPath);

    /*
     *  4. Search for "a/b/c" in EJSPATH
     */
    searchPath = mprStrdup(ejs, ejsPath);
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, slashName);
        if ((*path = probe(ejs, fileName)) != 0) {
            mprLog(ejs, 7, "Found %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(searchPath);

    /*
     *  5. Search for "c" in EJSPATH
     */
    baseName = mprGetBaseName(slashName);
    searchPath = mprStrdup(ejs, ejsPath);
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, baseName);
        if ((*path = probe(ejs, fileName)) != 0) {
            mprLog(ejs, 7, "Found %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }

    mprFree(searchPath);
    mprFree(slashName);

    return MPR_ERR_NOT_FOUND;
}


/*
 *  Read a string constant. String constants are stored as token offsets into
 *  the constant pool. The pool contains null terminated UTF-8 strings.
 *  TODO - rename ejsReadModuleString
 */
char *ejsModuleReadString(Ejs *ejs, EjsModule *mp)
{
    int     t;

    mprAssert(mp);

    if (ejsModuleReadNumber(ejs, mp, &t) < 0) {
        return 0;
    }
    return tokenToString(mp, t);
}


/*
 *  Read a type reference. Types are stored as either global property slot numbers or as strings (token offsets into the 
 *  constant pool). The lowest bit is set if the reference is a string. The type and name arguments are optional and may 
 *  be set to null. Return EJS_ERR for errors, otherwise 0. Return the 0 if successful, otherwise return EJS_ERR. If the 
 *  type could not be resolved, allocate a fixup record and return in *fixup. The caller should then call addFixup.
 */
//  TODO - must support reading an empty type to mean untyped.
//  TODO - this routine need much more error checking.

int ejsModuleReadType(Ejs *ejs, EjsModule *mp, EjsType **typeRef, EjsTypeFixup **fixup, EjsName *typeName, int *slotNum)
{
    EjsType         *type;
    EjsName         qname;
    int             t, slot;

    mprAssert(mp);
    mprAssert(typeRef);
    mprAssert(fixup);

    *typeRef = 0;
    *fixup = 0;

    if (typeName) {
        typeName->name = 0;
        typeName->space = 0;
    }

    if (ejsModuleReadNumber(ejs, mp, &t) < 0) {
        mprAssert(0);
        return EJS_ERR;
    }

    slot = -1;
    qname.name = 0;
    qname.space = 0;
    type = 0;

    switch (t & EJS_ENCODE_GLOBAL_MASK) {
    default:
        mp->hasError = 1;
        mprAssert(0);
        return EJS_ERR;

    case EJS_ENCODE_GLOBAL_NOREF:
        return 0;

    case EJS_ENCODE_GLOBAL_SLOT:
        /*
         *  Type is a builtin primitive type or we are binding globals.
         */
        slot = t >> 2;
        if (0 <= slot && slot < ejsGetPropertyCount(ejs, ejs->global)) {
            type = (EjsType*) ejsGetProperty(ejs, ejs->global, slot);
            if (type) {
                qname = type->qname;
            }
        }
        break;

    case EJS_ENCODE_GLOBAL_NAME:
        /*
         *  Type was unbound at compile time
         */
        qname.name = tokenToString(mp, t >> 2);
        if (qname.name == 0) {
            mp->hasError = 1;
            mprAssert(0);
            //  TODO - DIAG
            return EJS_ERR;
        }
        if ((qname.space = ejsModuleReadString(ejs, mp)) == 0) {
            mp->hasError = 1;
            mprAssert(0);
            return EJS_ERR;
        }
        if (qname.name) {
            slot = ejsLookupProperty(ejs, ejs->global, &qname);
            if (slot >= 0) {
                type = (EjsType*) ejsGetProperty(ejs, ejs->global, slot);
            }
        }
        break;
    }

    if (type) {
        if (!ejsIsType(type)) {
            mp->hasError = 1;
            mprAssert(0);
            return EJS_ERR;
        }
        *typeRef = type;

    } else if (type == 0 && fixup) {
        *fixup = createFixup(ejs, &qname, slot);
    }

    if (typeName) {
        *typeName = qname;
    }
    if (slotNum) {
        *slotNum = slot;
    }

    return 0;
}


static EjsTypeFixup *createFixup(Ejs *ejs, EjsName *qname, int slotNum)
{
    EjsTypeFixup    *fixup;

    fixup = mprAllocZeroed(ejs->typeFixups, sizeof(EjsTypeFixup));
    if (fixup == 0) {
        return 0;
    }
    fixup->typeName = *qname;
    fixup->typeSlotNum = slotNum;
    return fixup;
}


/*
 *  Convert a token index into a string.
 */
static char *tokenToString(EjsModule *mp, int token)
{
    if (token < 0 || token >= mp->constants->len) {
        mprAssert(0);
        return 0;
    }

    mprAssert(mp->constants);
    if (mp->constants == 0) {
        mprAssert(0);
        return 0;
    }

    return &mp->constants->pool[token];
}


/*
 *  Read an encoded number. Numbers are little-endian encoded in 7 bits with
 *  the 0x80 bit of each byte being a continuation bit.
 */
static int readNumber(Ejs *ejs, MprFile *file, int *number)
{
    int         c, t;

    mprAssert(file);
    mprAssert(number);

    if ((c = mprGetc(file)) < 0) {
        return MPR_ERR_CANT_READ;
    }

    t = c & 0x7f;

    if (c & 0x80) {
        if ((c = mprGetc(file)) < 0) {
            return MPR_ERR_CANT_READ;
        }
        t |= ((c &0x7f) << 7);

        if (c & 0x80) {
            if ((c = mprGetc(file)) < 0) {
                return MPR_ERR_CANT_READ;
            }
            t |= ((c &0x7f) << 14);

            if (c & 0x80) {
                if ((c = mprGetc(file)) < 0) {
                    return MPR_ERR_CANT_READ;
                }
                t |= ((c &0x7f) << 21);

                if (c & 0x80) {
                    if ((c = mprGetc(file)) < 0) {
                        return MPR_ERR_CANT_READ;
                    }
                    t |= ((c &0x7f) << 28);
                }
            }
        }
    }

    *number = t;
    return 0;
}


/*
 *  Read an encoded number. Numbers are little-endian encoded in 7 bits with
 *  the 0x80 bit of each byte being a continuation bit.
 */
int ejsModuleReadNumber(Ejs *ejs, EjsModule *mp, int *number)
{
    mprAssert(ejs);
    mprAssert(mp);
    mprAssert(number);

    if (readNumber(ejs, mp->file, number) < 0) {
        mp->hasError = 1;
        return -1;
    }
    return 0;
}


int ejsModuleReadByte(Ejs *ejs, EjsModule *mp, int *number)
{
    int     c;

    mprAssert(mp);
    mprAssert(number);

    // return mprGetc(mp->file);
    if ((c = mprGetc(mp->file)) < 0) {
        mp->hasError = 1;
        return MPR_ERR_CANT_READ;
    }
    *number = c;

    return 0;
}


static int addFixup(Ejs *ejs, int kind, EjsVar *target, int slotNum, EjsTypeFixup *fixup)
{
    int     index;

    mprAssert(ejs);
    mprAssert(fixup);

    fixup->kind = kind;
    fixup->target = target;
    fixup->slotNum = slotNum;

    index = mprAddItem(ejs->typeFixups, fixup);
    if (index < 0) {
        mprAssert(0);
        return EJS_ERR;
    }
    return 0;
}


#if BLD_FEATURE_EJS_DOC
static void setDoc(Ejs *ejs, EjsModule *mp, EjsVar *block, int slotNum)
{
    if (mp->doc && ejsIsBlock(block)) {
        ejsCreateDoc(ejs, (EjsBlock*) block, slotNum, mp->doc);
        mp->doc = 0;
    }
}


EjsDoc *ejsCreateDoc(Ejs *ejs, EjsBlock *block, int slotNum, cchar *docString)
{
    EjsDoc      *doc;
    char        key[32];

    //  TODO OPT - don't zero
    doc = mprAllocZeroed(ejs, sizeof(EjsDoc));
    if (doc == 0) {
        return 0;
    }

    doc->docString = mprStrdup(doc, docString);

    if (ejs->doc == 0) {
        ejs->doc = mprCreateHash(ejs, EJS_DOC_HASH_SIZE);
    }

    /*
     *  This is slow, but not critical path
     */
    mprSprintf(key, sizeof(key), "%Lx %d", PTOL(block), slotNum);
    mprAddHash(ejs->doc, key, doc);

    return doc;
}
#endif


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsLoader.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsModule.c"
 */
/************************************************************************/

/**
 *  ejsModule.c - Ejscript module management
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Lookup a module name in the set of loaded modules
 */
EjsModule *ejsLookupModule(Ejs *ejs, cchar *name)
{
    EjsModule   *mp;
    int         next;

    for (next = 0; (mp = (EjsModule*) mprGetNextItem(ejs->modules, &next)) != 0; ) {
        if (strcmp(mp->name, name) == 0) {
            return mp;
        }
    }
    return 0;
}



int ejsAddModule(Ejs *ejs, EjsModule *mp)
{
    mprAssert(ejs->modules);
    return mprAddItem(ejs->modules, mp);
}



int ejsRemoveModule(Ejs *ejs, EjsModule *mp)
{
    mprAssert(ejs->modules);
    return mprRemoveItem(ejs->modules, mp);
}



MprList *ejsGetModuleList(Ejs *ejs)
{
    return ejs->modules;
}



int ejsCheckModuleLoaded(Ejs *ejs, cchar *name)
{
    EjsModule       *mp;

    mp = (EjsModule*) ejsLookupModule(ejs, name);

    if (mp) {
        if (mp->loaded) {
            return 1;
        }
        if (mp->compiling && strcmp(name, EJS_DEFAULT_MODULE) != 0) {
            ejsThrowStateError(ejs, "Attempt to load module \"%s\" that is currently being compiled.", name);
            return EJS_ERR;
        }
    }
    return 0;
}


/*
 *  TODO - do we really need the name as an arg here or can they be defined by their property names.
 */
EjsModule *ejsCreateModule(Ejs *ejs, cchar *name, cchar *url, cchar *pool, int poolSize)
{
    EjsModule   *mp;

    /*
     *  We can't use ejsCreateType as our instances need to be EjsModules and not just EjsTypes.
     */
    mp = (EjsModule*) mprAllocZeroed(ejs, sizeof(EjsModule));
    if (mp == 0) {
        mprAssert(mp);
        return 0;
    }

    //  TODO OPT - should these be interned
    mp->name = mprStrdup(mp, name);
    mp->url = mprStrdup(mp, url);

    //  TODO - manage the versions somewhere
    //  TODO - warn when running wrong version
    mp->version = 1;

    //  TODO - don't zero
    mp->constants = mprAllocZeroed(mp, sizeof(EjsConst));
    if (mp->constants == 0) {
        return 0;
    }

    //  TODO - should only be created by the compiler
    mp->constants->table = mprCreateHash(mp->constants, 0);

    if (pool) {
        mprStealBlock(mp, pool);
        mp->constants->pool = (char*) pool;
        mp->constants->size = poolSize;
        mp->constants->len = poolSize;
    }

    mp->scopeChain = ejs->globalBlock;

    return mp;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsModule.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsScope.c"
 */
/************************************************************************/

/**
 *  ejsScope.c - Lookup variables in the scope chain.
 *
 *  This modules provides scope chain management including lookup, get and set services for variables. It will 
 *  lookup variables using the current execution variable scope and the set of open namespaces.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



static int ejsLookupVarInNamespaces(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);

/*
 *  Look for a variable by name in the scope chain and return the location in "lookup" and a positive slot number if found. 
 *  If the name.space is non-null/non-empty, then only the given namespace will be used. otherwise the set of open 
 *  namespaces will be used. The lookup structure will contain details about the location of the variable.
 */
int ejsLookupScope(Ejs *ejs, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsFrame        *frame;
    EjsBlock        *block;
    int             slotNum, nth;

    mprAssert(ejs);
    mprAssert(name);
    mprAssert(lookup);

    slotNum = -1;
    frame = ejs->frame;

    /*
     *  Look for the name in the scope chain considering each block scope. LookupVar will consider base classes and 
     *  namespaces. Don't search the last scope chain entry which will be global. For cloned interpreters, global 
     *  will belong to the master interpreter, so we must do that explicitly below to get the right global.
     */
    for (nth = 0, block = &frame->function.block; block->scopeChain; block = block->scopeChain) {

        if (block == (EjsBlock*) frame->thisObj->type) {
            /*
             *  This will lookup the instance and all base classes
             */
            if ((slotNum = ejsLookupVar(ejs, frame->thisObj, name, anySpace, lookup)) >= 0) {
                lookup->nthBlock = nth;
                break;
            }
            
        } else {
            if ((slotNum = ejsLookupVar(ejs, (EjsVar*) block, name, anySpace, lookup)) >= 0) {
                lookup->nthBlock = nth;
                break;
            }
        }
        nth++;
    }

    if (slotNum < 0 && ((slotNum = ejsLookupVar(ejs, ejs->global, name, anySpace, lookup)) >= 0)) {
        lookup->nthBlock = nth;
    }

    lookup->slotNum = slotNum;

    return slotNum;
}


/*
 *  Find a property in an object or type and its base classes.
 */
int ejsLookupVar(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsType     *type;
    int         slotNum;

    mprAssert(vp);
    mprAssert(vp->type);
    mprAssert(name);

    /*
     *  OPT - bit field initialization
     */
    lookup->nthBase = 0;
    lookup->nthBlock = 0;
    lookup->useThis = 0;
    lookup->instanceProperty = 0;
    lookup->ownerIsType = 0;

    /*
     *  Search through the inheritance chain of base classes. nthBase counts the subtypes that must be traversed. 
     */
    for (slotNum = -1, lookup->nthBase = 0; vp; lookup->nthBase++) {

        if ((slotNum = ejsLookupVarInBlock(ejs, vp, name, anySpace, lookup)) >= 0) {
            break;
        }

        /*
         *  Follow the base type chain. If an instance, first base type is vp->type.
         */
        vp = (vp->isType) ? (EjsVar*) ((EjsType*) vp)->baseType: (EjsVar*) vp->type;
        type = (EjsType*) vp;
        if (type == 0 || type->skipScope) {
            break;
        }
    }

    return lookup->slotNum = slotNum;
}


/*
 *  Find a variable in a block. Scope blocks are provided by the global object, types, functions and statement blocks.
 */
int ejsLookupVarInBlock(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    int     slotNum;

    mprAssert(vp);
    mprAssert(name);
    mprAssert(name->name);
    mprAssert(name->space);
    mprAssert(lookup);

    /*
     *  This code is ordered according the the frequency of various patterns of access. Change with care!
     */
    lookup->obj = vp;
    lookup->name = *name;

    if (name->space[0] || !anySpace) {
        /*
         *  If doing a search with an explicit namespace or not doing a namespace search or the type doesn't have or use
         *  namespaces, then ignore the set of open namespaces.
         */
        return ejsLookupProperty(ejs, vp, name);

    } else if (ejsIsObject(vp) && !ejsIsArray(vp)) {            //  TODO - fix this Array
        /*
         *  Fast lookup. This will return a valid slot number ONLY if there is only one property of the given name AND 
         *  that property has been defined with a standard (global) namespace. This catches 90% of scope lookups.
         */
        slotNum = ejsLookupSingleProperty(ejs, (EjsObject*) vp, &lookup->name);
        if (slotNum >= -1) {
            return slotNum;
        }
    }

    return ejsLookupVarInNamespaces(ejs, vp, name, anySpace, lookup);
}


/*
 *  Find a variable in a block. Scope blocks are provided by the global object, types, functions and statement blocks.
 */
static int ejsLookupVarInNamespaces(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsNamespace    *nsp;
    EjsName         qname;
    EjsBlock        *block, *currentBlock, *b;
    EjsVar          *owner;
    int             slotNum, nextNsp;

    mprAssert(vp);
    mprAssert(name);
    mprAssert(name->name);
    mprAssert(name->space);
    mprAssert(lookup);

    slotNum = -1;
    qname = *name;

    currentBlock = &ejs->frame->function.block;
    for (block = currentBlock; block; block = block->scopeChain) {

        for (b = currentBlock; b; b = b->scopeChain) {
            for (nextNsp = -1; (nsp = (EjsNamespace*) ejsGetPrevItem(&b->namespaces, &nextNsp)) != 0; ) {

                if (nsp->flags & EJS_NSP_PROTECTED && vp->isType) {
                    /*
                     *  Protected access. See if the type containing the method we are executing is a sub class of the type 
                     *  containing the property ie. Can we see protected properties?
                     */
                    owner = (EjsVar*) ejs->frame->function.owner;
                    if (owner && !ejsIsA(ejs, owner, (EjsType*) vp)) {
                        continue;
                    }
                }

                qname.space = nsp->uri;
                slotNum = ejsLookupProperty(ejs, vp, &qname);
                if (slotNum >= 0) {
                    lookup->name = qname;
                    lookup->obj = vp;
                    lookup->slotNum = slotNum;
                    return slotNum;
                }
            }
            if (b == block) {
                break;
            }
        }
    }
    return -1;
}


/*
 *  Get a variable by name. If vp is specified, it contains an explicit object in which to search for the variable name. 
 *  Otherwise, the full execution scope is consulted. The lookup fields will be set as residuals.
 */
EjsVar *ejsGetVarByName(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsVar  *result;
    int     slotNum;

    mprAssert(ejs);
    mprAssert(name);

    //  TODO - really nice to remove this
    if (vp && vp->type->helpers->getPropertyByName) {
        result = (*vp->type->helpers->getPropertyByName)(ejs, vp, name);
        if (result) {
            return result;
        }
    }

    if (vp) {
        slotNum = ejsLookupVar(ejs, vp, name, anySpace, lookup);
    } else {
        slotNum = ejsLookupScope(ejs, name, anySpace, lookup);
    }
    if (slotNum < 0) {
        return ejs->undefinedValue;
    }
    return ejsGetProperty(ejs, lookup->obj, slotNum);
}


void ejsShowBlockScope(Ejs *ejs, EjsBlock *block)
{
#if BLD_DEBUG
    EjsNamespace    *nsp;
    EjsList         *namespaces;
    int             nextNsp;

    mprLog(ejs, 6, "\n  Block scope");
    for (; block; block = block->scopeChain) {
        mprLog(ejs, 6, "    Block \"%s\" 0x%08x", block->obj.var.debugName, block);
        namespaces = &block->namespaces;
        if (namespaces) {
            for (nextNsp = 0; (nsp = (EjsNamespace*) ejsGetNextItem(namespaces, &nextNsp)) != 0; ) {
                mprLog(ejs, 6, "        \"%s\"", nsp->uri);
            }
        }
    }
#endif
}


void ejsShowCurrentScope(Ejs *ejs)
{
#if BLD_DEBUG
    EjsNamespace    *nsp;
    EjsList         *namespaces;
    EjsBlock        *block;
    int             nextNsp;

    if (ejs->frame == 0) {
        return;
    }

    mprLog(ejs, 6, "\n  Current scope");
    for (block = &ejs->frame->function.block; block; block = block->scopeChain) {
        mprLog(ejs, 6, "    Block \"%s\" 0x%08x", block->obj.var.debugName, block);
        namespaces = &block->namespaces;
        if (namespaces) {
            for (nextNsp = 0; (nsp = (EjsNamespace*) ejsGetNextItem(namespaces, &nextNsp)) != 0; ) {
                mprLog(ejs, 6, "        \"%s\"", nsp->uri);
            }
        }
    }
#endif
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsScope.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsService.c"
 */
/************************************************************************/

/**
 *  ejsService.c - Ejscript interpreter factory
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_WEB
#endif


static void allocNotifier(Ejs *ejs, uint size, uint total, bool granted);
static int  cloneMaster(Ejs *ejs, Ejs *master);
static int  configureEjsModule(Ejs *ejs, EjsModule *mp, cchar *path);
static int  defineTypes(Ejs *ejs);
static void defineHelpers(Ejs *ejs);
static void destroyEjs(Ejs *ejs);
static int  runSpecificMethod(Ejs *ejs, cchar *className, cchar *methodName);
static int  searchForMethod(Ejs *ejs, cchar *methodName, EjsType **typeReturn);
static void setEjsPath(EjsService *sp);

#if BLD_FEATURE_STATIC || BLD_FEATURE_EJS_ALL_IN_ONE
#if BLD_FEATURE_EJS_DB
static int configureDbModule(Ejs *ejs, EjsModule *mp, cchar *path);
#endif
#if BLD_FEATURE_EJS_WEB
static int configureWebModule(Ejs *ejs, EjsModule *mp, cchar *path);
#endif
#endif

/*
 *  Global singleton for the Ejs service
 */
EjsService *_globalEjsService;

/*
 *  Initialize the EJS subsystem
 */
EjsService *ejsCreateService(MprCtx ctx)
{
    EjsService  *sp;

    sp = mprAllocObjZeroed(ctx, EjsService);
    if (sp == 0) {
        return 0;
    }
    _globalEjsService = sp;

    setEjsPath(sp);
    return sp;
}


Ejs *ejsCreate(MprCtx ctx, Ejs *master, int flags)
{
    Ejs     *ejs;

    /*
     *  Create interpreter structure
     */
    ejs = mprAllocObjWithDestructorZeroed(ctx, Ejs, destroyEjs);
    if (ejs == 0) {
        return 0;
    }
    mprSetAllocNotifier(ejs, (MprAllocNotifier) allocNotifier);

    ejs->service = _globalEjsService;
    ejs->flags |= (flags & (EJS_FLAG_EMPTY | EJS_FLAG_COMPILER | EJS_FLAG_NO_EXE | EJS_FLAG_DOC));

    if (ejsInitStack(ejs) < 0) {
        mprFree(ejs);
        return 0;
    }

    ejsCreateGCService(ejs);

    if (master == 0) {
        ejs->modules = mprCreateList(ejs);
        ejs->coreTypes = mprCreateHash(ejs, 0);
        ejs->nativeModules = mprCreateHash(ejs, 0);
        ejs->standardSpaces = mprCreateHash(ejs, 0);
        defineHelpers(ejs);
        if (defineTypes(ejs) < 0) {
            mprFree(ejs);
            return 0;
        }

    } else {
        cloneMaster(ejs, master);
    }

    if (mprHasAllocError(ejs)) {
        mprError(ejs, "Memory allocation error during initialization");
        mprFree(ejs);
        return 0;
    }

    ejs->initialized = 1;
    ejsCollectGarbage(ejs, EJS_GC_ALL);

    ejsSetGeneration(ejs, EJS_GEN_NEW);

    return ejs;
}


static void destroyEjs(Ejs *ejs)
{
    if (ejs->stack.bottom) {
        mprMapFree(ejs->stack.bottom, ejs->stack.size);
    }
}


static void defineHelpers(Ejs *ejs)
{
    ejs->defaultHelpers = (EjsTypeHelpers*) mprAllocZeroed(ejs, sizeof(EjsTypeHelpers));
    ejsInitializeDefaultHelpers(ejs->defaultHelpers);

    /*
     *  Object inherits the default helpers. Block inherits the object helpers
     */
    ejs->objectHelpers = (EjsTypeHelpers*) mprMemdup(ejs, (void*) ejs->defaultHelpers, sizeof(EjsTypeHelpers));
    ejsInitializeObjectHelpers(ejs->objectHelpers);

    ejs->blockHelpers = (EjsTypeHelpers*) mprMemdup(ejs, (void*) ejs->objectHelpers, sizeof(EjsTypeHelpers));
    ejsInitializeBlockHelpers(ejs->blockHelpers);
}


/*
 *  Create the core language types. These are native types and are created prior to loading ejs.mod.
 *  The loader then matches these types to the loaded definitions.
 */
static int createTypes(Ejs *ejs)
{
    /*
     *  Create the essential bootstrap types: Object, Type and the global object, these are the foundation.
     *  All types are instances of Type. Order matters here.
     */
    ejsCreateObjectType(ejs);
    ejsCreateTypeType(ejs);
    ejsCreateBlockType(ejs);
    ejsCreateNamespaceType(ejs);
    ejsCreateFunctionType(ejs);
    ejsCreateGlobalBlock(ejs);
    ejsCreateNullType(ejs);

    /*
     *  Now create the rest of the native types. Order does not matter.
     */
    ejsCreateArrayType(ejs);
    ejsCreateBlockType(ejs);
    ejsCreateBooleanType(ejs);
    ejsCreateByteArrayType(ejs);
    ejsCreateDateType(ejs);
    ejsCreateErrorType(ejs);
    ejsCreateIteratorType(ejs);
    ejsCreateNumberType(ejs);
    ejsCreateReflectType(ejs);
    ejsCreateStringType(ejs);
    ejsCreateVoidType(ejs);
#if ES_XML && BLD_FEATURE_EJS_E4X
    ejsCreateXMLType(ejs);
#endif
#if ES_XMLList && BLD_FEATURE_EJS_E4X
    ejsCreateXMLListType(ejs);
#endif
#if ES_RegExp && BLD_FEATURE_REGEXP
    ejsCreateRegExpType(ejs);
#endif
    ejsCreateAppType(ejs);
    ejsCreateConfigType(ejs);
    ejsCreateGCType(ejs);
    ejsCreateMemoryType(ejs);
    ejsCreateSystemType(ejs);
#if ES_ejs_events_Timer
    ejsCreateTimerType(ejs);
#endif
#if ES_ejs_io_File
    ejsCreateFileType(ejs);
#endif
#if ES_ejs_io_Http && BLD_FEATURE_HTTP_CLIENT
    ejsCreateHttpType(ejs);
#endif

    /*
     *  The native module callbacks are invoked after loading the mod file. This allows the callback routines to configure
     *  native methods and do other native type adjustments. configureEjsModule is always invoked even if SHARED because it
     *  is never loaded from a shared library. Normally, when loading from a shared library, the init routine is invoked
     *  immediately after loading the mod file and it should call the configuration routine.
     */
    ejsAddNativeModule(ejs, "ejs", configureEjsModule);

#if BLD_FEATURE_STATIC || BLD_FEATURE_EJS_ALL_IN_ONE
    if (!(ejs->flags & EJS_FLAG_EMPTY)) {
#if BLD_FEATURE_EJS_DB
        ejsAddNativeModule(ejs, "ejs.db", configureDbModule);
#endif
#if BLD_FEATURE_EJS_WEB
        ejsAddNativeModule(ejs, "ejs.web", configureWebModule);
#endif
    }
#endif

    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        return MPR_ERR;
    }
    return 0;
}


/*
 *  This will configure all the core types by defining native methods and properties
 */
static int configureEjsModule(Ejs *ejs, EjsModule *mp, cchar *path)
{
    EjsModule   *pp;
    
    if (ejs->flags & EJS_FLAG_EMPTY) {
        return 0;
    }

#if UNUSED
#if _ES_CHECKSUM_ejs
    if (mp->seq != _ES_CHECKSUM_ejs) {
        ejsThrowIOError(ejs, "Module \"%s\" does not match native code", path);
        return EJS_ERR;
    }
#endif
#endif

    /*
     *  Order matters. Put in dependency order
     */
    ejsConfigureObjectType(ejs);
    ejsConfigureArrayType(ejs);
    ejsConfigureBlockType(ejs);
    ejsConfigureBooleanType(ejs);
    ejsConfigureByteArrayType(ejs);
    ejsConfigureDateType(ejs);
    ejsConfigureFunctionType(ejs);
    ejsConfigureGlobalBlock(ejs);
    ejsConfigureErrorType(ejs);
    ejsConfigureIteratorType(ejs);
    ejsConfigureNamespaceType(ejs);
    ejsConfigureNumberType(ejs);
    ejsConfigureNullType(ejs);
    ejsConfigureReflectType(ejs);
    ejsConfigureStringType(ejs);
    ejsConfigureTypeType(ejs);
    ejsConfigureVoidType(ejs);
#if ES_XML && BLD_FEATURE_EJS_E4X
    ejsConfigureXMLType(ejs);
#endif
#if ES_XMLList && BLD_FEATURE_EJS_E4X
    ejsConfigureXMLListType(ejs);
#endif
#if ES_RegExp && BLD_FEATURE_REGEXP
    ejsConfigureRegExpType(ejs);
#endif

    ejsConfigureAppType(ejs);
    ejsConfigureConfigType(ejs);
    ejsConfigureGCType(ejs);
    ejsConfigureMemoryType(ejs);
    ejsConfigureSystemType(ejs);
#if ES_ejs_events_Timer
    ejsConfigureTimerType(ejs);
#endif
#if ES_ejs_io_File
    ejsConfigureFileType(ejs);
#endif
#if ES_ejs_io_Http && BLD_FEATURE_HTTP_CLIENT
    ejsConfigureHttpType(ejs);
#endif

    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        mprAssert(0);
        return MPR_ERR;
    }
    
    if ((pp = ejsLookupModule(ejs, "ejs.events")) != 0) {
        pp->configured = 1;
    }
    if ((pp = ejsLookupModule(ejs, "ejs.sys")) != 0) {
        pp->configured = 1;
    }
    if ((pp = ejsLookupModule(ejs, "ejs.io")) != 0) {
        pp->configured = 1;
    }
    return 0;
}


#if BLD_FEATURE_STATIC || BLD_FEATURE_EJS_ALL_IN_ONE
#if BLD_FEATURE_EJS_DB
static int configureDbModule(Ejs *ejs, EjsModule *mp, cchar *path)
{
    ejsConfigureDbTypes(ejs);
    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        mprAssert(0);
        return MPR_ERR;
    }
    mp->configured = 1;
    return 0;
}
#endif


#if BLD_FEATURE_EJS_WEB
static int configureWebModule(Ejs *ejs, EjsModule *mp, cchar *path)
{
    ejsConfigureWebTypes(ejs);

    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        mprAssert(0);
        return MPR_ERR;
    }
    mp->configured = 1;
    return 0;
}
#endif
#endif


/*
 *  Register a native module callback to be invoked when it it time to configure the module. This is used by loadable modules
 *  when they are built statically.
 */
int ejsAddNativeModule(Ejs *ejs, char *name, EjsNativeCallback callback)
{
    if (mprAddHash(ejs->nativeModules, name, callback) == 0) {
        return EJS_ERR;
    }
    return 0;
}


static int defineTypes(Ejs *ejs)
{
    /*
     *  Create all the builtin types. These are defined and hashed. Not defined in global.
     */
    if (createTypes(ejs) < 0 || ejs->hasError) {
        mprError(ejs, "Can't create core types");
        return EJS_ERR;
    }

    /*
     *  Load the builtin module. This will create all the type definitions and match with builtin native types.
     *  This will call the configure routines defined in moduleConfig and will run the module initializers.
     */
    if (! (ejs->flags & EJS_FLAG_EMPTY)) {
        if (ejsLoadModule(ejs, EJS_MOD, NULL, NULL, EJS_MODULE_BUILTIN) == 0) {
            mprError(ejs, "Can't load " EJS_MOD);
            return EJS_ERR;
        }
    }

    return 0;
}


#if BLD_DEBUG
/*
 *  TODO - remove this
 */
static void checkType(Ejs *ejs, EjsType *type)
{
    EjsVar          *vp;
    EjsTrait        *trait;
    EjsName         qname;
    int             i, count;
    
    i = type->block.obj.var.dynamic;
    mprAssert(type->block.obj.var.dynamic == 0);
    mprAssert(i == 0);
    
    count = ejsGetPropertyCount(ejs, (EjsVar*) type);
    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, (EjsVar*) type, i);
/* TODO - temp workaround */
        if (vp == 0 || vp->type == 0) {
            continue;
        }
        if (ejsIsFunction(vp)) {
            continue;
        }
        trait = ejsGetTrait((EjsBlock*) type, i);
        if (trait == 0) {
            continue;
        }
        if (trait->attributes & EJS_ATTR_CONST) {
            continue;
        } else {
            qname = ejsGetPropertyName(ejs, (EjsVar*) type, i);
            if (qname.name && qname.name[0] == '\0') {
                continue;
            }
            mprLog(ejs, 6, "Found non-const property %s.%s", type->qname.name, qname.name);
        }
    }
    //  TODO - got to test all properties of the type
}
#endif


static int cloneMaster(Ejs *ejs, Ejs *master)
{
    EjsName     qname;
    EjsType     *type;
    EjsVar      *vp;
    EjsTrait    *trait;
    int         i, count;

    mprAssert(master);

    ejs->master = master;
    ejs->service = master->service;
    ejs->defaultHelpers = master->defaultHelpers;
    ejs->objectHelpers = master->objectHelpers;
    ejs->blockHelpers = master->blockHelpers;
    ejs->objectType = master->objectType;

    ejs->arrayType = master->arrayType;
    ejs->blockType = master->blockType;
    ejs->booleanType = master->booleanType;
    ejs->byteArrayType = master->byteArrayType;
    ejs->dateType = master->dateType;
    ejs->errorType = master->errorType;
    ejs->functionType = master->functionType;
    ejs->iteratorType = master->iteratorType;
    ejs->namespaceType = master->namespaceType;
    ejs->nullType = master->nullType;
    ejs->numberType = master->numberType;
    ejs->objectType = master->objectType;
    ejs->regExpType = master->regExpType;
    ejs->stringType = master->stringType;
    ejs->stopIterationType = master->stopIterationType;
    ejs->typeType = master->typeType;
    ejs->voidType = master->voidType;

#if BLD_FEATURE_EJS_E4X
    ejs->xmlType = master->xmlType;
    ejs->xmlListType = master->xmlListType;
#endif

    ejs->emptyStringValue = master->emptyStringValue;
    ejs->falseValue = master->falseValue;
    ejs->infinityValue = master->infinityValue;
    ejs->minusOneValue = master->minusOneValue;
    ejs->nanValue = master->nanValue;
    ejs->negativeInfinityValue = master->negativeInfinityValue;
    ejs->nullValue = master->nullValue;
    ejs->oneValue = master->oneValue;
    ejs->trueValue = master->trueValue;
    ejs->undefinedValue = master->undefinedValue;
    ejs->zeroValue = master->zeroValue;

    ejs->configSpace = master->configSpace;
    ejs->emptySpace = master->emptySpace;
    ejs->eventsSpace = master->eventsSpace;
    ejs->ioSpace = master->ioSpace;
    ejs->intrinsicSpace = master->intrinsicSpace;
    ejs->iteratorSpace = master->iteratorSpace;
    ejs->internalSpace = master->internalSpace;
    ejs->publicSpace = master->publicSpace;
    ejs->sysSpace = master->sysSpace;

    ejs->argv = master->argv;
    ejs->argc = master->argc;
    ejs->coreTypes = master->coreTypes;
    ejs->standardSpaces = master->standardSpaces;

    ejs->modules = mprDupList(ejs, master->modules);

    //  Push this code into ejsGlobal.c. Call ejsCloneGlobal
    
    ejs->globalBlock = ejsCreateBlock(ejs, EJS_GLOBAL, master->globalBlock->obj.capacity);
    ejs->global = (EjsVar*) ejs->globalBlock; 
    ejs->globalBlock->obj.numProp = master->globalBlock->obj.numProp;
    ejsGrowBlock(ejs, ejs->globalBlock, ejs->globalBlock->obj.numProp);
    
    ejsCopyList(ejs->globalBlock, &ejs->globalBlock->namespaces, &master->globalBlock->namespaces);

    /*
     *  TODO - some form of copy on write or 2 level global would be better.
     *  TODO - OPT. Could accelerate this. Push into ejsGlobal.c and do block copies of slots, traits etc and then rehash.
     */
    count = ejsGetPropertyCount(master, master->global);
    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, master->global, i);
        if (vp) {
            //TODO  mprAssert(vp->generation == EJS_GEN_ETERNAL || vp == master->global);
            ejsSetProperty(ejs, ejs->global, i, ejsGetProperty(master, master->global, i));
            qname = ejsGetPropertyName(master, master->global, i);
            ejsSetPropertyName(ejs, ejs->global, i, &qname);
            trait = ejsGetTrait(master->globalBlock, i);
            ejsSetTrait(ejs->globalBlock, i, trait->type, trait->attributes);
#if BLD_DEBUG
            //  TODO - remove
            if (ejsIsType(vp) && vp != (EjsVar*) ejs->objectType) {
                checkType(ejs, (EjsType*) vp);
            }
#endif
        }
    }
    
    /*
     *  Clone some mutable types
     *  TODO - remove this. Must handle mutable types better.
     */
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_XML); 
    ejsSetProperty(ejs, ejs->global, ES_XML, ejsCloneVar(ejs, (EjsVar*) type, 0));

#if FUTURE
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_sys_GC); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_sys_GC, ejsCloneVar(ejs, (EjsVar*) type, 0));     
#endif
     
#if ES_ejs_db_Database
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_db_Database); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_db_Database, ejsCloneVar(ejs, (EjsVar*) type, 0));
#else
    /*
     *  Building shared
     */
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.db", "Database"));
    ejsSetPropertyByName(ejs, ejs->global, &qname, ejsCloneVar(ejs, (EjsVar*) type, 0));
#endif

#if UNUSED
#if ES_ejs_db_Record
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_db_Record); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_db_Record, ejsCloneVar(ejs, (EjsVar*) type, 0));
#endif
#endif

#if ES_ejs_web_GoogleConnector
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_web_GoogleConnector); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_web_GoogleConnector, ejsCloneVar(ejs, (EjsVar*) type, 0));
#else
    /*
     *  Shared. TODO - this is just for the Google.nextId
     */
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "GoogleConnector")); 
    ejsSetPropertyByName(ejs, ejs->global, &qname, ejsCloneVar(ejs, (EjsVar*) type, 0));
#endif
     
    ejsSetProperty(ejs, ejs->global, ES_global, ejs->global);

    return 0;
}


/*
 *  Notifier callback function. Invoked by mprAlloc on allocation errors
 */
static void allocNotifier(Ejs *ejs, uint size, uint total, bool granted)
{
    ejs->attention = 1;
}


/*
 *  Prepend a search path to the system defaults
 */
void ejsSetSearchPath(EjsService *sp, cchar *newpath)
{
    char    *syspath;

    mprAssert(sp);
    mprAssert(sp->ejsPath);

    setEjsPath(sp);
    syspath = sp->ejsPath;

    mprAllocSprintf(sp, &sp->ejsPath, -1, "%s" MPR_SEARCH_DELIM "%s", newpath, syspath);
    mprFree(syspath);
    mprLog(sp, 4, "Search path set to %s", sp->ejsPath);
}


static void setEjsPath(EjsService *sp)
{
    char    *search, *env, *modDir, dir[MPR_MAX_FNAME];

    mprGetAppDir(sp, dir, sizeof(dir));

    mprAllocSprintf(sp, &modDir, -1, "%s/../lib/modules", dir);

    mprAllocSprintf(sp, &search, -1, 
        "%s" MPR_SEARCH_DELIM "%s" MPR_SEARCH_DELIM "%s" MPR_SEARCH_DELIM "%s" MPR_SEARCH_DELIM ".", 
        dir, mprCleanFilename(modDir, modDir), BLD_MOD_PREFIX, BLD_ABS_MOD_DIR);
    mprFree(modDir);

    env = getenv("EJSPATH");
    if (env && *env) {
        mprAllocSprintf(sp, &search, -1, "%s" MPR_SEARCH_DELIM "%s", env, search);
#if FUTURE
        putenv(env);
#endif
    }
    mprFree(sp->ejsPath);
    sp->ejsPath = mprStrdup(sp, search);
    mprMapDelimiters(sp, sp->ejsPath, '/');
}


EjsVar *ejsGetGlobalObject(Ejs *ejs)
{
    return (EjsVar*) ejs->global;
}


void ejsSetHandle(Ejs *ejs, void *handle)
{
    ejs->handle = handle;
}


void *ejsGetHandle(Ejs *ejs)
{
    return ejs->handle;
}


#if BLD_FEATURE_MULTITHREAD && FUTURE
void ejsSetServiceLocks(EjsService *sp, EjsLockFn lock, EjsUnlockFn unlock, void *data)
{
    mprAssert(sp);

    sp->lock = lock;
    sp->unlock = unlock;
    sp->lockData = data;
    return 0;
}
#endif


/*
 *  Evaluate a module file
 */
int ejsEvalModule(cchar *path)
{
    EjsService      *vm;   
    Ejs             *ejs;
    Mpr             *mpr;

    mpr = mprCreate(0, NULL, NULL);
    if ((vm = ejsCreateService(mpr)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if ((ejs = ejsCreate(vm, NULL, EJS_FLAG_COMPILER)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if (ejsLoadModule(ejs, path, NULL, NULL, 0) < 0) {
        mprFree(mpr);
        return MPR_ERR_CANT_READ;
    }
    if (ejsRun(ejs) < 0) {
        mprFree(mpr);
        return EJS_ERR;
    }
    mprFree(mpr);
    return 0;
}


/*
 *  Run a program. This will run a program assuming that all the required modules are already loaded. It will
 *  optionally service events until instructed to exit.
 */
int ejsRunProgram(Ejs *ejs, cchar *className, cchar *methodName)
{
    /*
     *  Run all module initialization code. This includes plain old scripts.
     */
    if (ejsRun(ejs) < 0) {
        return EJS_ERR;
    }

    /*
     *  Run the requested method. This will block until completion
     */
    if (className || methodName) {
        if (runSpecificMethod(ejs, className, methodName) < 0) {
            return EJS_ERR;
        }
    }

    if (ejs->flags & EJS_FLAG_NOEXIT) {
        /*
         *  This will service events until App.exit() is called
         */
        mprServiceEvents(ejs, -1, 0);
    }

    return 0;
}


/*
 *  Run the specified method in the named class. If methodName is null, default to "main".
 *  If className is null, search for the first class containing the method name.
 */
static int runSpecificMethod(Ejs *ejs, cchar *className, cchar *methodName)
{
    EjsType         *type;
    EjsFunction     *fun;
    EjsName         qname;
    EjsVar          *args;
    int             attributes, i;

    /*
     *  TODO - this is bugged. What about package names? MethodSlot and type are not being set in some cases.
     */
    type = 0;

    if (className == 0 && methodName == 0) {
        return 0;
    }

    if (methodName == 0) {
        methodName = "main";
    }

    /*
     *  Search for the first class with the given name
     */
    if (className == 0) {
        if (searchForMethod(ejs, methodName, &type) < 0) {
            return EJS_ERR;
        }

    } else {
        ejsName(&qname, EJS_PUBLIC_NAMESPACE, className);
        type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &qname);
    }

    if (type == 0 || !ejsIsType(type)) {
        mprError(ejs, "Can't find class \"%s\"", className);
        return EJS_ERR;
    }

    ejsName(&qname, EJS_PUBLIC_NAMESPACE, methodName);
    fun = (EjsFunction*) ejsGetPropertyByName(ejs, (EjsVar*) type, &qname);
    if (fun == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    if (! ejsIsFunction(fun)) {
        mprError(ejs, "Property \"%s\" is not a function");
        return MPR_ERR_BAD_STATE;
    }

    attributes = ejsGetTypePropertyAttributes(ejs, (EjsVar*) type, fun->slotNum);
    if (!(attributes & EJS_ATTR_STATIC)) {
        mprError(ejs, "Method \"%s\" is not declared static");
        return EJS_ERR;
    }

    args = (EjsVar*) ejsCreateArray(ejs, ejs->argc);
    for (i = 0; i < ejs->argc; i++) {
        ejsSetProperty(ejs, args, i, (EjsVar*) ejsCreateString(ejs, ejs->argv[i]));
    }

    if (ejsRunFunction(ejs, fun, 0, 1, &args) == 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Search for the named method in all types.
 */
static int searchForMethod(Ejs *ejs, cchar *methodName, EjsType **typeReturn)
{
    EjsFunction *method;
    EjsType     *type;
    EjsName     qname;
    EjsVar      *global, *vp;
    int         globalCount, slotNum, methodCount;
    int         methodSlot;

    mprAssert(methodName && *methodName);
    mprAssert(typeReturn);

    global = ejs->global;
    globalCount = ejsGetPropertyCount(ejs, global);

    /*
     *  Search for the named method in all types
     */
    for (slotNum = 0; slotNum < globalCount; slotNum++) {
        vp = ejsGetProperty(ejs, global, slotNum);
        if (vp == 0 || !ejsIsType(vp)) {
            continue;
        }
        type = (EjsType*) vp;

        methodCount = ejsGetPropertyCount(ejs, (EjsVar*) type);

        for (methodSlot = 0; methodSlot < methodCount; methodSlot++) {
            method = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, methodSlot);
            if (method == 0) {
                continue;
            }

            qname = ejsGetPropertyName(ejs, (EjsVar*) type, methodSlot);
            if (qname.name && strcmp(qname.name, methodName) == 0) {
                *typeReturn = type;
            }
        }
    }
    return 0;
}


static void logHandler(MprCtx ctx, int flags, int level, const char *msg)
{
    Mpr         *mpr;
    MprFile     *file;
    char        *prefix;

    mpr = mprGetMpr(ctx);
    file = (MprFile*) mpr->logHandlerData;
    prefix = mpr->name;

    while (*msg == '\n') {
        mprFprintf(file, "\n");
        msg++;
    }

    if (flags & MPR_LOG_SRC) {
        mprFprintf(file, "%s: %d: %s\n", prefix, level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        /*
         *  Use static printing to avoid malloc when the messages are small.
         *  This is important for memory allocation errors.
         */
        if (strlen(msg) < (MPR_MAX_STRING - 32)) {
            mprStaticErrorPrintf(file, "%s: Error: %s\n", prefix, msg);
        } else {
            mprFprintf(file, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprFprintf(file, "%s: Fatal: %s\n", prefix, msg);
        
    } else if (flags & MPR_RAW) {
        mprFprintf(file, "%s", msg);
    }
}


int ejsStartLogging(Mpr *mpr, char *logSpec)
{
    MprFile     *file;
    char        *levelSpec;
    int         level;

    level = 0;
    logSpec = mprStrdup(mpr, logSpec);

    //  TODO - move should not be changinging logSpec.
    if ((levelSpec = strchr(logSpec, ':')) != 0) {
        *levelSpec++ = '\0';
        level = atoi(levelSpec);
    }

    if (strcmp(logSpec, "stdout") == 0) {
        file = mpr->fileService->console;

    } else if (strcmp(logSpec, "stderr") == 0) {
        file = mpr->fileService->error;

    } else {
        if ((file = mprOpen(mpr, logSpec, O_CREAT | O_WRONLY | O_TRUNC | O_TEXT, 0664)) == 0) {
            mprErrorPrintf(mpr, "Can't open log file %s\n", logSpec);
            mprFree(logSpec);
            return EJS_ERR;
        }
    }

    mprSetLogLevel(mpr, level);
    mprSetLogHandler(mpr, logHandler, (void*) file);

    mprFree(logSpec);
    return 0;
}


/*
 *  Global memory allocation handler
 */
void ejsMemoryFailure(MprCtx ctx, uint size, uint total, bool granted)
{
    if (!granted) {
        //  TODO use mprPrintError
        mprPrintf(ctx, "Can't allocate memory block of size %d\n", size);
        mprPrintf(ctx, "Total memory used %d\n", total);
        exit(255);
    }
    mprPrintf(ctx, "Memory request for %d bytes exceeds memory red-line\n", size);
    mprPrintf(ctx, "Total memory used %d\n", total);

    //  TODO - should we not do something more here (run GC if running?)
}


void ejsReportError(Ejs *ejs, char *fmt, ...)
{
    va_list     arg;
    const char  *msg;
    char        *buf;

    va_start(arg, fmt);
    
    mprAllocVsprintf(ejs, &buf, 0, fmt, arg);
    
    /*
     *  Compiler error format is:
     *      program:line:errorCode:SEVERITY: message
     *  Where program is either "ec" or "ejs"
     *  Where SEVERITY is either "error" or "warn"
     */
    msg = ejsGetErrorMsg(ejs, 1);
    
    mprError(ejs, "%s", (msg) ? msg: buf);
    mprFree(buf);
    va_end(arg);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsService.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsTrace.c"
 */
/************************************************************************/

#if FUTURE
/*
 *  @file       ejsTrace.c
 *  @brief      JIT Tracing
 *  @overview 
 *  @remarks 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  Next
 *  - Need to have stackDepth counter and do auto pop like ecCodeGen does       
 *  - Need register allocation
 *  - Need guards 
 *
 *  Questions
 *      How to handle exceptions
 *          - Throwing routine must handle the stack for compiled code
 */




//  NEED to age spots down every so often
#define EJS_JIT_HOT_SPOT        5
#define EJS_JIT_HASH            4099


typedef struct EjsRecord {
    int         pc;                                 /* Should this be a ptr ? */
    int         opcode;

    union {
        struct {
            EjsName     qname;
            int         slotNum;
            int         nthBase;
        } name;

        struct {
            EjsVar      value;
        } literalGetter;
    };

    struct EjsTrace     *trace;                     /* Owning trace */

} EjsRecord;


typedef struct EjsTrace {
    int             origin;                         //  TODO - shoudl this be a pointer?
    MprList         *records;                       /* Trace records */


    struct EjsTrace *parent;
    struct EjsTrace *forw;
    struct EjsTrace *back;
} EjsTrace;



typedef struct EjsJit {
    MprList         *traces;
    EjsTrace        *currentTrace;
    EjsTrace        preCompile;                     /*  Traces to be compiled */
    EjsTrace        postCompile;                    /*  Compiled traces */
    char            hotSpotHash[EJS_JIT_HASH]       /* Hash of PC locations */
} EjsJit;



/*
 */
EjsJit *ejsCreateJit(Ejs *ejs)
{
    EjsJit  *jit;

    jit = mprAllocObjZeroed(ctx, EjsJit);
    if (jit == 0) {
        return 0;
    }
    jit->traces = mprCreateList(jit);
    
    return jit;
}



static EjsTrace *createTrace(EjsJit *jit)
{
    EjsTrace    *trace;

    trace = mprAllocObjZeroed(jit, EjsTrace);
    if (trace == 0) {
        return 0;
    }
    return trace;
}



void queueTrace(EjsTrace *head, EjsTrace *trace)
{
    trace = head->forw;

    //  TODO - how to do this lock free??
    trace->forw = head;
    trace->back = head->back;
    trace->back->forw = trace;
    head->back = trace;
}



EjsTrace *dequeTrace(EjsTrace *head)
{
    EjsTrace    *trace;

    //  TODO - how to do this lock free??
    trace = head->forw;
    head->forw = trace->forw;
    head->forw->back = head;
}



static int createTraceRecord(EjsTrace *trace, int origin)
{
    EjsRecord       *rec;
    
    rec = mprAllocObjZeroed(trace, EjsRecord);
    if (rec == 0) {
        return 0;
    }
    rec->origin = origin;
    return rec;
}



/*
 *  Need to age down the temp of all spots
 */
int getSpotTemp(EjsJit *jit, int spot)
{
    return hotSpotHash[(spot >> 4) % EJS_JIT_HASH];
}



void heatSpot(EjsJit *jit, int spot)
{
    return hotSpotHash[(spot >> 4) % EJS_JIT_HASH]++;
}



void removeSpot(EjsJit *jit, int spot)
{
    return hotSpotHash[(spot >> 4) % EJS_JIT_HASH] = 0;
}



/*
 *  TODO Do we need to make sure this is a backward branch?
 */
void ejsTraceBranch(Ejs *ejs, int pc)
{
    EjsJit      *jit;
    EjsTrace    *trace;
    EjsRecord   *rec;

    jit = ejs->jit;

    trace = lookupTrace(jit, pc);
    if (trace == 0) {
        temp = getLocationTemp(jit, pc);
        if (temp < EJS_JIT_HOT_SPOT) {
            heatLocation(jit, pc);
            return;
        }
        trace = createTrace(jit);
        if (trace == 0) {
            return;
        }
    }

    if (trace->pc != pc) {
        /* Must wait until the first trace is compiled */
        return;
    }

    if (trace->pc == pc) {
        /* Trace is complete */
        queueTrace(&jit->preCompile, trace);
        removeSpot(pc);
        ejsCompileTraces(ejs);
    }
}



EjsTraceRecord *createRecord(Ejs *ejs, int pc)
{
    EjsTrace    *trace;
    EjsRecord   *record;

    if ((trace = jit->currentTrace) == 0) {
        return 0;
    }

    rec = createTraceRecord(trace, pc);
    if (rec == 0) {
        return 0;
    }
    rec->trace = trace;

    if (mprAddItem(trace->records, rec) < 0) {
        mprFree(rec);
        ejs->jit->currentTrace = 0;
        mprFree(trace);
    }

    return rec;
}



/*
 *  TODO what about nthBase
 */
void ejsTraceLoadProp(Ejs *ejs, int pc, int opcode, int slotNum, EjsName qname)
{
    EjsTraceRecord  *rec;

    if ((rec = createRecord(ejs, pc)) == 0) {
        return;
    }

    rec->opcode = opcode;
    rec->name.slotNum = slotNum;
    rec->name.qname = qname;
    rec->name.nthBase = nthBase;
}



/*
 *  TODO - how to encode all iteral values
 */
void ejsTraceLoadLiteral(Ejs *ejs, int pc, int opcode, int value)
{
    EjsTraceRecord  *rec;

    if ((rec = createRecord(ejs, pc)) == 0) {
        return;
    }
    rec->opcode = opcode;
    rec->literalGetter.value = value;
}



void ejsCompileTraces(Ejs *ejs)
{
    EjsJit          *jit;
    EjsTrace        *trace;
    EjsTraceRecord  *rec;
    int             next;

    //  TODO - Can we do this lock free?
    while ((trace = dequeueTrace(&jit->preCompile)) != 0) {
        for (next = -1; rec = mprGetPrevItem(trace->records, &next)) != 0; ) {
            switch (rec->opcode) {
            case EJS_OP_LOAD_INT_8:
                /*
                 *  push(ejs, ejsCreateInteger(ejs, getByte(frame)));
                 */
                encodeInvoke(ejs, EJS_FN_CREATE_INTEGER, rec->value, 1);
                encodePushAccum(ejs, );
                break;

            default:
                interpretRecord(ejs, rec);
            }
        }
    }
}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *
 *  @end
 */
#endif
void __dummyEjsTrace() {}
/************************************************************************/
/*
 *  End of file "../vm/ejsTrace.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../vm/ejsVar.c"
 */
/************************************************************************/

/**
 *  ejsVar.c - Helper methods for the ejsVar interface.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static MprNumber parseNumber(Ejs *ejs, cchar *str, bool isHex);
static bool      parseBoolean(Ejs *ejs, cchar *s);

/**
 *  Cast the variable to a given target type.
 *  @return Returns a variable with the result of the cast or null if an exception is thrown.
 */
EjsVar *ejsCastVar(Ejs *ejs, EjsVar *vp, EjsType *targetType)
{
    mprAssert(ejs);
    mprAssert(targetType);

    if (vp == 0) {
        vp = ejs->undefinedValue;
    }
    if (vp->type == targetType) {
        return vp;
    }
    if (vp->type->helpers->castVar) {
        return (vp->type->helpers->castVar)(ejs, vp, targetType);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/*
 *  Create a new instance of a variable. Delegate to the type specific create.
 */
EjsVar *ejsCreateVar(Ejs *ejs, EjsType *type, int numSlots)
{
    /*
     *  The VxWorks cc386 invoked linker crashes without this test
     */
    if (type == 0) {
        return 0;
    }
    if (type->helpers->createVar) {
        return (type->helpers->createVar)(ejs, type, numSlots);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", type->qname.name);
    return 0;
}

/**
 *  Copy a variable by copying all properties. If a property is a reference  type, just copy the reference.
 *  See ejsDeepClone for a complete recursive copy of all reference contents.
 *  @return Returns a variable or null if an exception is thrown.
 */
EjsVar *ejsCloneVar(Ejs *ejs, EjsVar *vp, bool deep)
{
    if (vp == 0) {
        return 0;
    }
    if (vp->type->helpers->cloneVar) {
        return (vp->type->helpers->cloneVar)(ejs, vp, deep);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/*
 *  Define a property and its traits.
 *  @return Return the slot number allocated for the property.
 */
int ejsDefineProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *name, EjsType *propType, int attributes, EjsVar *value)
{
    mprAssert(name);
    mprAssert(name->name);
    mprAssert(name->space);
    
    if (vp->type->helpers->defineProperty) {
        return (vp->type->helpers->defineProperty)(ejs, vp, slotNum, name, propType, attributes, value);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/**
 *  Delete a property in an object variable. The stack is unchanged.
 *  @return Returns a status code.
 */
int ejsDeleteProperty(Ejs *ejs, EjsVar *vp, int slot)
{
    mprAssert(slot >= 0);
    
    if (vp->type->helpers->deleteProperty) {
        return (vp->type->helpers->deleteProperty)(ejs, vp, slot);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return EJS_ERR;
}


/**
 *  Delete a property in an object variable. The stack is unchanged.
 *  @return Returns a status code.
 */
int ejsDeletePropertyByName(Ejs *ejs, EjsVar *vp, EjsName *qname)
{
    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(qname->space);
    
    if (vp->type->helpers->deletePropertyByName) {
        return (vp->type->helpers->deletePropertyByName)(ejs, vp, qname);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return EJS_ERR;
}


//  TODO -- who should be calling ejsDestroy and when

void ejsDestroyVar(Ejs *ejs, EjsVar *vp)
{
    EjsType     *type;

    mprAssert(vp);

    type = vp->type;

    mprAssert(type->helpers->destroyVar);
    (type->helpers->destroyVar)(ejs, vp);
}


/**
 *  Finalize an object by calling the finalize method if one exists.
 *  @return Returns a status code.
 */
void ejsFinalizeVar(Ejs *ejs, EjsVar *vp)
{
    mprAssert(ejs);
    mprAssert(vp);

    if (vp->type->helpers->finalizeVar) {
        (vp->type->helpers->finalizeVar)(ejs, vp);
    }
}


/**
 *  Get a property at a given slot in a variable.
 *  @return Returns the requested property varaible.
 */
EjsVar *ejsGetProperty(Ejs *ejs, EjsVar *vp, int slotNum)
{
    mprAssert(ejs);
    mprAssert(vp);
    mprAssert(slotNum >= 0);

    if (vp->type->helpers->getProperty) {
        return (vp->type->helpers->getProperty)(ejs, vp, slotNum);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/*
 *  Get a property given a name.
 */
EjsVar *ejsGetPropertyByName(Ejs *ejs, EjsVar *vp, EjsName *name)
{
    int     slotNum;

    mprAssert(ejs);
    mprAssert(vp);
    mprAssert(name);

    if (vp->type->helpers->getPropertyByName) {
        return (vp->type->helpers->getPropertyByName)(ejs, vp, name);
    }

    /*
     *  Fall back and use a two-step lookup and get
     */
    slotNum = ejsLookupProperty(ejs, vp, name);
    if (slotNum < 0) {
        return 0;
    }
    return ejsGetProperty(ejs, vp, slotNum);
}


/**
 *  Return the number of properties in the variable.
 *  @return Returns the number of properties.
 */
int ejsGetPropertyCount(Ejs *ejs, EjsVar *vp)
{
    if (vp->type->helpers->getPropertyCount) {
        return (vp->type->helpers->getPropertyCount)(ejs, vp);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return EJS_ERR;
}


/**
 *  Return the name of a property indexed by slotNum.
 *  @return Returns the property name.
 */
EjsName ejsGetPropertyName(Ejs *ejs, EjsVar *vp, int slotNum)
{
    EjsName     qname;

    if (vp->type->helpers->getPropertyName) {
        return (vp->type->helpers->getPropertyName)(ejs, vp, slotNum);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);

    qname.name = 0;
    qname.space = 0;
    return qname;
}


/**
 *  Return the trait for the indexed by slotNum.
 *  @return Returns the property name.
 */
EjsTrait *ejsGetPropertyTrait(Ejs *ejs, EjsVar *vp, int slotNum)
{
    if (vp->type->helpers->getPropertyTrait) {
        return (vp->type->helpers->getPropertyTrait)(ejs, vp, slotNum);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/**
 *  Get a property slot. Lookup a property name and return the slot reference. If a namespace is supplied, the property
 *  must be defined with the same namespace.
 *  @return Returns the slot number or -1 if it does not exist.
 */
int ejsLookupProperty(Ejs *ejs, EjsVar *vp, EjsName *name)
{
    mprAssert(ejs);
    mprAssert(vp);
    mprAssert(name);
    mprAssert(name->name);
    mprAssert(name->space);

    if (vp->type->helpers->lookupProperty) {
        return (vp->type->helpers->lookupProperty)(ejs, vp, name);
    }
    /*
     *  No throw so types can omit implementing lookupProperty if they only implement getPropertyByName
     */
    return EJS_ERR;
}


/*
 *  Invoke an operator.
 *  vp is left-hand-side
 *  @return Return a variable with the result or null if an exception is thrown.
 */
EjsVar *ejsInvokeOperator(Ejs *ejs, EjsVar *vp, int opCode, EjsVar *rhs)
{
    mprAssert(vp);

    if (vp) {
        if (vp->type->helpers->invokeOperator) {
            return (vp->type->helpers->invokeOperator)(ejs, vp, opCode, rhs);
        }
    }
    if (ejs->exception == NULL) {
        ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    }
    return 0;
}


/*
 *  ejsMarkVar is in ejsGarbage.c
 */


/*
 *  Set a property and return the slot number. Incoming slot may be -1 to allocate a new slot.
 */
int ejsSetProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *value)
{
    if (vp == 0) {
        ejsThrowReferenceError(ejs, "Object is null");
        return EJS_ERR;
    }

    if (vp->type->helpers->setProperty) {
        ejsSetReference(ejs, vp, value);
        return (vp->type->helpers->setProperty)(ejs, vp, slotNum, value);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return EJS_ERR;
}


/*
 *  Set a property given a name.
 */
int ejsSetPropertyByName(Ejs *ejs, EjsVar *vp, EjsName *qname, EjsVar *value)
{
    int     slotNum;

    mprAssert(ejs);
    mprAssert(vp);
    mprAssert(qname);

    if (vp->type->helpers->setPropertyByName) {
        ejsSetReference(ejs, vp, value);
        return (vp->type->helpers->setPropertyByName)(ejs, vp, qname, value);
    }

    /*
     *  Fall back and use a two-step lookup and get
     */
    slotNum = ejsLookupProperty(ejs, vp, qname);
    if (slotNum < 0) {
        slotNum = ejsSetProperty(ejs, vp, -1, value);
        if (slotNum < 0) {
            return EJS_ERR;
        }
        if (ejsSetPropertyName(ejs, vp, slotNum, qname) < 0) {
            return EJS_ERR;
        }
        return slotNum;
    }
    ejsSetReference(ejs, vp, value);
    return ejsSetProperty(ejs, vp, slotNum, value);
}


/*
 *  Set the property name and return the slot number. Slot may be -1 to allocate a new slot.
 */
int ejsSetPropertyName(Ejs *ejs, EjsVar *vp, int slot, EjsName *qname)
{
    if (vp->type->helpers->setPropertyName) {
        return (vp->type->helpers->setPropertyName)(ejs, vp, slot, qname);
    }
    mprError(ejs, "Helper not defined for type");
    return EJS_ERR;
}


int ejsSetPropertyTrait(Ejs *ejs, EjsVar *vp, int slot, EjsType *propType, int attributes)
{
    if (vp->type->helpers->setPropertyTrait) {
        return (vp->type->helpers->setPropertyTrait)(ejs, vp, slot, propType, attributes);
    }
    mprError(ejs, "Helper not defined for type");
    return EJS_ERR;
}


/**
 *  Get a string representation of a variable.
 *  @return Returns a string variable or null if an exception is thrown.
 */
EjsString *ejsToString(Ejs *ejs, EjsVar *vp)
{
    if (vp == 0 || ejsIsString(vp)) {
        return (EjsString*) vp;
    }

    if (vp->type->helpers->castVar) {
        return (EjsString*) (vp->type->helpers->castVar)(ejs, vp, ejs->stringType);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/**
 *  Get a numeric representation of a variable.
 *  @return Returns a number variable or null if an exception is thrown.
 */
EjsNumber *ejsToNumber(Ejs *ejs, EjsVar *vp)
{
    if (vp == 0 || ejsIsNumber(vp)) {
        return (EjsNumber*) vp;
    }

    if (vp->type->helpers->castVar) {
        return (EjsNumber*) (vp->type->helpers->castVar)(ejs, vp, ejs->numberType);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/**
 *  Get a boolean representation of a variable.
 *  @return Returns a number variable or null if an exception is thrown.
 */
EjsBoolean *ejsToBoolean(Ejs *ejs, EjsVar *vp)
{
    if (vp == 0 || ejsIsBoolean(vp)) {
        return (EjsBoolean*) vp;
    }

    if (vp->type->helpers->castVar) {
        return (EjsBoolean*) (vp->type->helpers->castVar)(ejs, vp, ejs->booleanType);
    }
    ejsThrowInternalError(ejs, "Helper not defined for type \"%s\"", vp->type->qname.name);
    return 0;
}


/*
 *  Fully construct a new object. We create a new instance and call all required constructors.
 */

EjsVar *ejsCreateInstance(Ejs *ejs, EjsType *type, int argc, EjsVar **argv)
{
    EjsFunction     *fun;
    EjsVar          *vp;
    int             slotNum;

    mprAssert(type);

    vp = ejsCreateVar(ejs, type, 0);
    if (vp == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    if (type->hasConstructor) {
        slotNum = type->block.numInherited;
        fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, slotNum);
        if (fun == 0) {
            return 0;
        }
        if (!ejsIsFunction(fun)) {
            return 0;
        }

        vp->permanent = 1;
        ejsRunFunction(ejs, fun, vp, argc, argv);
        vp->permanent = 0;
    }

    return vp;
}


/*
 *  Report reference errors
 */
static void reportError(Ejs *ejs, EjsVar *vp)
{
    if (ejsIsNull(vp)) {
        ejsThrowReferenceError(ejs, "Object reference is null");

    } else if (ejsIsUndefined(vp)) {
        ejsThrowReferenceError(ejs, "Reference is undefined");

    } else {
        ejsThrowReferenceError(ejs, "Undefined setProperty helper");
    }
}


static EjsVar *createVar(Ejs *ejs, EjsType *type, int size)
{
    return ejsAllocVar(ejs, type, size);
}


static EjsVar *castVar(Ejs *ejs, EjsVar *vp, EjsType *toType)
{
    EjsString   *result;
    char        *buf;

    /*
     *  TODO - should support cast to Boolean and Number
     */
    mprAllocSprintf(ejs, &buf, 0, "[object %s]", vp->type->qname.name);
    result = ejsCreateString(ejs, buf);
    mprFree(buf);
    return (EjsVar*) result;

}


/*
 *  Default clone is just to do a shallow copy since most types are immutable.
 */
static EjsVar *cloneVar(Ejs *ejs, EjsVar *vp, bool depth)
{
    return vp;
}


static void destroyVar(Ejs *ejs, EjsVar *vp)
{
    ejsFreeVar(ejs, vp);
}


static int defineProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *name, EjsType *propType, int attributes, EjsVar *value)
{
    reportError(ejs, vp);
    return EJS_ERR;
}


static int deleteProperty(Ejs *ejs, EjsVar *vp, int slotNum)
{
    reportError(ejs, vp);
    return EJS_ERR;
}


static int finalizeVar(Ejs *ejs, EjsVar *vp)
{
    return 0;
}


static EjsVar *getProperty(Ejs *ejs, EjsVar *vp, int slotNum)
{
    reportError(ejs, vp);
    return 0;
}


static int getPropertyCount(Ejs *ejs, EjsVar *vp)
{
    return 0;
}


static EjsName getPropertyName(Ejs *ejs, EjsVar *vp, int slotNum)
{
    static EjsName  qname;

    reportError(ejs, vp);

    qname.name = 0;
    qname.space = 0;

    return qname;
}


static EjsTrait *getPropertyTrait(Ejs *ejs, EjsVar *vp, int slotNum)
{
    return ejsGetTrait((EjsBlock*) vp->type, slotNum);
}


static EjsVar *invokeOperator(Ejs *ejs, EjsVar *lhs, int opCode, EjsVar *rhs)
{
    switch (opCode) {

    case EJS_OP_BRANCH_EQ:
    case EJS_OP_BRANCH_STRICTLY_EQ:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs == rhs);

    case EJS_OP_BRANCH_NE:
    case EJS_OP_BRANCH_STRICTLY_NE:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs == rhs));

    default:
        /*
         *  Pass to the standard Object helpers to implement Object methods.
         */
        return (ejs->objectHelpers->invokeOperator)(ejs, lhs, opCode, rhs);
    }
    return 0;
}


static int lookupProperty(Ejs *ejs, EjsVar *vp, EjsName *name)
{
    /*
     *  Don't throw or report error if lookupProperty is not implemented
     */
    return EJS_ERR;
}


static void markVar(Ejs *ejs, EjsVar *parent, EjsVar *vp)
{
}


static int setProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *value)
{
    reportError(ejs, vp);
    return EJS_ERR;
}


static int setPropertyName(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *name)
{
    reportError(ejs, vp);
    return EJS_ERR;
}


static int setPropertyTrait(Ejs *ejs, EjsVar *vp, int slotNum, EjsType *propType, int attributes)
{
    reportError(ejs, vp);
    return EJS_ERR;
}


/*
 *  Native types are responsible for completing the rest of the helpers
 */
void ejsInitializeDefaultHelpers(EjsTypeHelpers *helpers)
{
    helpers->castVar            = castVar;
    helpers->createVar          = createVar;
    helpers->cloneVar           = cloneVar;
    helpers->destroyVar         = destroyVar;
    helpers->defineProperty     = defineProperty;
    helpers->deleteProperty     = deleteProperty;
    helpers->finalizeVar        = finalizeVar;
    helpers->getProperty        = getProperty;
    helpers->getPropertyTrait   = getPropertyTrait;
    helpers->getPropertyCount   = getPropertyCount;
    helpers->getPropertyName    = getPropertyName;
    helpers->invokeOperator     = invokeOperator;
    helpers->lookupProperty     = lookupProperty;
    helpers->markVar            = markVar;
    helpers->setProperty        = setProperty;
    helpers->setPropertyName    = setPropertyName;
    helpers->setPropertyTrait   = setPropertyTrait;
}


EjsTypeHelpers *ejsGetDefaultHelpers(Ejs *ejs)
{
    return (EjsTypeHelpers*) mprMemdup(ejs, ejs->defaultHelpers, sizeof(EjsTypeHelpers));
}


EjsTypeHelpers *ejsGetObjectHelpers(Ejs *ejs)
{
    /*
     *  TODO  OPT. Does every type need a copy -- NO. But the loader uses this and allows
     *  native types to selectively override. See samples/types/native.
     */
    return (EjsTypeHelpers*) mprMemdup(ejs, ejs->objectHelpers, sizeof(EjsTypeHelpers));
}


EjsTypeHelpers *ejsGetBlockHelpers(Ejs *ejs)
{
    return (EjsTypeHelpers*) mprMemdup(ejs, ejs->blockHelpers, sizeof(EjsTypeHelpers));
}



//  TODO - where to move this?
EjsName *ejsAllocName(MprCtx ctx, cchar *name, cchar *space)
{
    EjsName     *np;

    np = mprAllocObj(ctx, EjsName);
    if (np) {
        np->name = mprStrdup(np, name);
        np->space = mprStrdup(np, space);
    }
    return np;
}


EjsName ejsCopyName(MprCtx ctx, EjsName *qname)
{
    EjsName     name;

    name.name = mprStrdup(ctx, qname->name);
    name.space = mprStrdup(ctx, qname->space);

    return name;
}


EjsName *ejsDupName(MprCtx ctx, EjsName *qname)
{
    return ejsAllocName(ctx, qname->name, qname->space);
}


EjsName *ejsName(EjsName *np, cchar *space, cchar *name)
{
    np->name = name;
    np->space = space;
    return np;
}


void ejsZeroSlots(Ejs *ejs, EjsVar **slots, int count)
{
    int     i;

    for (i = 0; i < count; i++) {
        slots[i] = ejs->nullValue;
    }
}


/*
 *  Parse a string based on formatting instructions and intelligently create a variable.
 *
 *  Float and decimal format: [+|-]DIGITS][DIGITS][(e|E)[+|-]DIGITS]
 *
 *  TODO - refactor all this number parsing.
 */
EjsVar *ejsParseVar(Ejs *ejs, cchar *buf, int preferredType)
{
    cchar       *cp;
    int         isHex, type;

    mprAssert(buf);

    type = preferredType;
    isHex = 0;

    if (preferredType == ES_Void || preferredType < 0) {
        if (*buf == '-' || *buf == '+') {
            type = ejs->numberType->id;

        } else if (!isdigit((int) *buf)) {
            if (strcmp(buf, "true") == 0 || strcmp(buf, "false") == 0) {
                type = ES_Boolean;
            } else {
                type = ES_String;
            }

        } else {
            if (buf[0] == '0' && tolower((int) buf[1]) == 'x') {
                isHex = 1;
                for (cp = &buf[2]; *cp; cp++) {
                    if (! isxdigit((int) *cp)) {
                        break;
                    }
                }

            } else {
                for (cp = buf; *cp; cp++) {
                    if (! isdigit((int) *cp)) {
#if BLD_FEATURE_FLOATING_POINT
                        int c = tolower((int) *cp);
                        if (c != '.' && c != 'e' && c != 'f')
#endif
                            break;
                    }
                }
            }
            if (*cp == '\0') {
                type = ES_Number;
            } else {
                type = ES_String;
            }
        }
    }

    switch (type) {
    case ES_Object:
    case ES_Void:
    case ES_Null:
    default:
        break;

    case ES_Number:
        return (EjsVar*) ejsCreateNumber(ejs, parseNumber(ejs, buf, isHex));

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, parseBoolean(ejs, buf));

    case ES_String:
        if (strcmp(buf, "null") == 0) {
            return (EjsVar*) ejsCreateNull(ejs);

        } else if (strcmp(buf, "undefined") == 0) {
            return (EjsVar*) ejsCreateUndefined(ejs);
        }

        return (EjsVar*) ejsCreateString(ejs, buf);
    }
    return (EjsVar*) ejsCreateUndefined(ejs);
}


/*
 *  Convert the variable to a number type. Only works for primitive types.
 */
static bool parseBoolean(Ejs *ejs, cchar *s)
{
    if (s == 0 || *s == '\0') {
        return 0;
    }
    if (strcmp(s, "false") == 0 || strcmp(s, "FALSE") == 0) {
        return 0;
    }
    return 1;
}


/*
 *  Convert the string buffer to a Number.
 */
static MprNumber parseNumber(Ejs *ejs, cchar *str, bool isHex)
{
    cchar   *cp;
    int64   num;
    int     radix, c, negative;

    mprAssert(str);

#if BLD_FEATURE_FLOATING_POINT
    //  TODO - refactor all this number parsing
    if (!isHex && !(str[0] == '0' && isdigit((int) str[1]))) {
        return atof(str);
    }
#endif

    cp = str;
    num = 0;
    negative = 0;

    if (*cp == '-') {
        cp++;
        negative = 1;
    } else if (*cp == '+') {
        cp++;
    }

    /*
     *  Parse a number. Observe hex and octal prefixes (0x, 0)
     */
    if (*cp != '0') {
        /*
         *  Normal numbers (Radix 10)
         */
        while (isdigit((int) *cp)) {
            num = (*cp - '0') + (num * 10);
            cp++;
        }
    } else {
        cp++;
        if (tolower((int) *cp) == 'x') {
            cp++;
            radix = 16;
            while (*cp) {
                c = tolower((int) *cp);
                if (isdigit(c)) {
                    num = (c - '0') + (num * radix);
                } else if (c >= 'a' && c <= 'f') {
                    num = (c - 'a' + 10) + (num * radix);
                } else {
                    break;
                }
                cp++;
            }

        } else{
            radix = 8;
            while (*cp) {
                c = tolower((int) *cp);
                if (isdigit(c) && c < '8') {
                    num = (c - '0') + (num * radix);
                } else {
                    break;
                }
                cp++;
            }
        }
    }

    if (negative) {
        return (MprNumber) (0 - num);
    }
    return (MprNumber) num;
}

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../vm/ejsVar.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../db/ejsDb.c"
 */
/************************************************************************/

/*
 *  ejsDb.c -- Database class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *  This file is currently hard-code for SQLite. In the future, it will be split into
 *  a generic adapter top-end and a set of database adapters.
 *
 *  Todo:
 *      - should handle SQLITE_BUSY for multiuser access. Need to set the default timeout
 */



#include    "sqlite3.h"

#if BLD_FEATURE_EJS_DB


typedef struct EjsDb
{
    EjsObject       obj;
    sqlite3         *sdb;               /* Sqlite handle */
    MprHeap         *arena;             /* Memory context arena */
    MprThreadLocal  *tls;               /* Thread local data for setting Sqlite memory context */
} EjsDb;



static int dbDestructor(EjsDb **db);

/*
 *  DB Constructor and also used for constructor for sub classes.
 *
 *  function DB(connectionString: String)
 */
static EjsVar *dbConstructor(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    sqlite3     *sdb;
    EjsDb       **dbp;
    char        *path;

    path = ejsGetString(argv[0]);    
    
    /*
     *  Create a memory context for use by sqlite. This is a virtual paged memory region.
     *  TODO - this is not ideal for long running applications.
     */
    db->arena = mprAllocArena(ejs, "sqlite", EJS_MAX_DB_MEM, 0, 0);
    if (db->arena == 0) {
        return 0;
    }
    
    /*
     *  Create a destructor object so we can cleanup and close the database. Must create after the arena so it will be
     *  invoked before the arena is freed. 
     */
    if ((dbp = mprAllocObject(ejs, 1, (MprDestructor) dbDestructor)) == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }
    *dbp = db;
    
    db->tls = mprCreateThreadLocal(db->arena);
    if (db->tls == 0) {
        return 0;
    }
    ejsSetDbMemoryContext(db->tls, db->arena);

    sdb = 0;
    if (sqlite3_open(path, &sdb /* TODO remove , SQLITE_OPEN_READWRITE, 0 */) != SQLITE_OK) {
        ejsThrowIOError(ejs, "Can't open database %s", path);
        return 0;
    }
    db->sdb = sdb;

    sqlite3_busy_timeout(sdb, 15000);

    /*
     *  Query or change the count-changes flag. Normally, when the count-changes flag is not set, INSERT, UPDATE and
     *  DELETE statements return no data. When count-changes is set, each of these commands returns a single row of
     *  data consisting of one integer value - the number of rows inserted, modified or deleted by the command. The
     *  returned change count does not include any insertions, modifications or deletions performed by triggers.
     */
//  sqlite3_exec(sdb, "PRAGMA count_changes = OFF", NULL, NULL, NULL);

    ejsSetProperty(ejs, (EjsVar*) db, ES_ejs_db_Database__connection, (EjsVar*) ejsCreateString(ejs, path));
    ejsSetProperty(ejs, (EjsVar*) db, ES_ejs_db_Database__name, (EjsVar*) ejsCreateString(ejs, mprGetBaseName(path)));
    
    return 0;
}


static int dbDestructor(EjsDb **dbp)
{
    EjsDb       *db;

    db = *dbp;

    if (db->sdb) {
        ejsSetDbMemoryContext(db->tls, db->arena);
        sqlite3_close(db->sdb);
        db->sdb = 0;
    }
    return 0;
}


/*
 *  function close(): Void
 */
static int closeDb(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    mprAssert(ejs);
    mprAssert(db);

    if (db->sdb) {
        ejsSetDbMemoryContext(db->tls, db->arena);
        sqlite3_close(db->sdb);
        db->sdb = 0;
    }
    return 0;
}


#if UNUSED
static int commitDb(Ejs *ejs, EjsVar *db, int argc, EjsVar **argv)
{
    mprAssert(ejs);
    mprAssert(db);

    return 0;
}


static int rollbackDb(Ejs *ejs, EjsVar *db, int argc, EjsVar **argv)
{
    mprAssert(ejs);
    mprAssert(db);

    return 0;
}
#endif


/*
 *  function sql(cmd: String): Array
 *
 *  Will support multiple sql cmds but will only return one result table.
 */
static EjsVar *sql(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    sqlite3         *sdb;
    sqlite3_stmt    *stmt;
    EjsArray        *result;
    EjsVar          *row, *svalue;
    EjsName         qname;
    cchar           *tail, *colName, *cmd, *value;
    int             i, ncol, rc, retries, rowNum;

    mprAssert(ejs);
    mprAssert(db);

    cmd = ejsGetString(argv[0]);
    
    ejsSetDbMemoryContext(db->tls, db->arena);

    rc = SQLITE_OK;
    retries = 0;
    sdb = db->sdb;

    if (sdb == 0) {
        ejsThrowIOError(ejs, "Database is closed");
        return 0;
    }
    mprAssert(sdb);

    result = ejsCreateArray(ejs, 0);
    if (result == 0) {
        return 0;
    }

    while (cmd && *cmd && (rc == SQLITE_OK || (rc == SQLITE_SCHEMA && ++retries < 2))) {

        stmt = 0;
        rc = sqlite3_prepare_v2(sdb, cmd, -1, &stmt, &tail);
        if (rc != SQLITE_OK) {
            continue;
        }
        if (stmt == 0) {
            /* Comment or white space */
            cmd = tail;
            continue;
        }

        ncol = sqlite3_column_count(stmt);

        for (rowNum = 0; ; rowNum++) {

            rc = sqlite3_step(stmt);

            if (rc == SQLITE_ROW) {

                row = (EjsVar*) ejsCreateSimpleObject(ejs);
                if (row == 0) {
                    sqlite3_finalize(stmt);
                    return 0;
                }
                if (ejsSetProperty(ejs, (EjsVar*) result, rowNum, row) < 0) {
                    /* TODO rc */
                }
                for (i = 0; i < ncol; i++) {
                    colName = sqlite3_column_name(stmt, i);
                    value = (cchar*) sqlite3_column_text(stmt, i);
                    ejsName(&qname, EJS_EMPTY_NAMESPACE, mprStrdup(row, colName));
                    if (ejsLookupProperty(ejs, row, &qname) < 0) {
                        svalue = (EjsVar*) ejsCreateString(ejs, mprStrdup(row, value));
                        if (ejsSetPropertyByName(ejs, row, &qname, svalue) < 0) {
                            /* TODO */
                        }
                    }
                }
            } else {
                rc = sqlite3_finalize(stmt);
                stmt = 0;

                if (rc != SQLITE_SCHEMA) {
                    //  TODO - what is this?
                    retries = 0;
                    for (cmd = tail; isspace(*cmd); cmd++) {
                        ;
                    }
                }
                break;
            }
#if UNUSED
            /* TODO -- should we be doing this */
            cmd = tail;
#endif
        }
    }

    if (stmt) {
        rc = sqlite3_finalize(stmt);
    }

    if (rc != SQLITE_OK) {
        if (rc == sqlite3_errcode(sdb)) {
            ejsThrowIOError(ejs, "SQL error: %s", sqlite3_errmsg(sdb));
        } else {
            ejsThrowIOError(ejs, "Unspecified SQL error");
        }
        return 0;
    }

    return (EjsVar*) result;
}


#if UNUSED
/*
 *  Save database changes
 *
 *  function save(): Void
 */
static EjsVar *save(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    EjsArray    *ap;
    char        **data, *error;
    int         rc, rowCount, i;

    ejsSetDbMemoryContext(db->tls, db->arena);

    ap = ejsCreateArray(ejs, 0);

    rc = sqlite3_exec(db->sdb,
      "SELECT name FROM sqlite_master "
      "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'"
      "UNION ALL "
      "SELECT name FROM sqlite_temp_master "
      "WHERE type IN ('table','view') "
      "ORDER BY 1",
      0,  &error);

    if (error) {
        ejsThrowIOError(ejs, "%s", error);
        sqlite3_free(error);
    }
    if (rc == SQLITE_OK){
        for (i = 1; i <= rowCount; i++) {
            ejsSetProperty(ejs, (EjsVar*) ap, i - 1, (EjsVar*) ejsCreateString(ejs, data[i]));
        }
    }
    sqlite3_free_table(data);
    return (EjsVar*) ap;
}
#endif


#if UNUSED
/*
 *  Return an array of table names in the database.
 *  function get tables(): Array
 */
static EjsVar *tables(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    EjsArray    *ap;
    char        **data, *error;
    int         rc, rowCount, i;

    ejsSetDbMemoryContext(db->tls, db->arena);

    ap = ejsCreateArray(ejs, 0);

    rc = sqlite3_get_table(db->sdb,
      "SELECT name FROM sqlite_master "
      "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'"
      "UNION ALL "
      "SELECT name FROM sqlite_temp_master "
      "WHERE type IN ('table','view') "
      "ORDER BY 1",
      &data, &rowCount, 0, &error);

    if (error) {
        ejsThrowIOError(ejs, "%s", error);
        sqlite3_free(error);
    }
    if (rc == SQLITE_OK){
        for (i = 1; i <= rowCount; i++) {
            ejsSetProperty(ejs, (EjsVar*) ap, i - 1, (EjsVar*) ejsCreateString(ejs, data[i]));
        }
    }
    sqlite3_free_table(data);
    return (EjsVar*) ap;
}
#endif


/*
 *  Called by the garbage colllector
 */
static EjsVar *finalizeDb(Ejs *ejs, EjsDb *db)
{
    if (db->sdb) {
        ejsSetDbMemoryContext(db->tls, db->arena);
        closeDb(ejs, db, 0, 0);
        db->sdb = 0;
    }
    return 0;
}


void ejsConfigureDbTypes(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    ejsName(&qname, "ejs.db", "Database");
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &qname);
    if (type == 0 || !ejsIsType(type)) {
        ejs->hasError = 1;
        return;
    }

    type->instanceSize = sizeof(EjsDb);
    type->helpers->finalizeVar = (EjsFinalizeVarHelper) finalizeDb;

    ejsBindMethod(ejs, type, ES_ejs_db_Database_Database, (EjsNativeFunction) dbConstructor);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_close, (EjsNativeFunction) closeDb);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_sql, (EjsNativeFunction) sql);

#if UNUSED
    ejsSetAccessors(ejs, type, ES_ejs_db_Database_tables, (EjsNativeFunction) tables, -1, 0);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_start, startDb);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_commit, commitDb);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_rollback, rollbackDb);
#endif
}


/*
 *  Loadable module interface
 */
MprModule *ejs_dbModuleInit(Ejs *ejs)
{
    MprModule   *module;

    module = mprCreateModule(ejs, "db", BLD_VERSION, 0, 0, 0);
    if (module == 0) {
        return 0;
    }

    ejsSetGeneration(ejs, EJS_GEN_ETERNAL);
    ejsConfigureDbTypes(ejs);
    ejsSetGeneration(ejs, EJS_GEN_OLD);

    return module;
}


#endif /* BLD_FEATURE_EJS_DB */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../db/ejsDb.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebController.c"
 */
/************************************************************************/

/**
 *  ejsWebController.c - Native code for the Controller class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_WEB

/*
 *  Add a cache-contol header
 *
 *  function cache(enable: Boolean = true): Void
 */
static EjsVar *cache(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  Enable sessions for this request.
 *
 *  function createSession(timeout: Number = 0): String
 */
static EjsVar *createSession(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    int         timeout;

    mprAssert(argc == 0 || argc == 1);

    timeout = (argc == 1) ? ejsGetInt(argv[0]): 0;
    web = ejsGetHandle(ejs);

    return (EjsVar*) ejsCreateSession(ejs, timeout, 0 /* TODO - need secure arg */);
}


/*
 *  function destroySession(): Void
 */
static EjsVar *destroySession(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    ejsDestroySession(ejs);
#if FUTURE
    EjsWeb     *web;

    mprAssert(argc == 0);

    web = ejsGetHandle(ejs);
    web->control->destroySession(web->handle);
#endif
    return 0;
}


/*
 *  Discard all pending output to the client
 *
 *  function discardOutput(): Void
 */
static EjsVar *discardOutput(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    ejsDiscardOutput(ejs);
    return 0;
}


/*
 *  Send an error response back to the client.
 *
 *  function sendError(code: Number, msg: String): Void
 */
static EjsVar *sendError(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    cchar       *msg;
    int         code;

    mprAssert(argc == 2);

    web = ejs->handle;
    code = ejsGetInt(argv[0]);
    msg = ejsGetString(argv[1]);

    ejsSetHttpCode(ejs, code);

    if (web->flags & EJS_WEB_FLAG_BROWSER_ERRORS) {
        ejsDiscardOutput(ejs);
        ejsWrite(ejs, msg);

    } else {
        //  TODO - this erorr should not go into the general web log
        mprLog(web, 3, "Web request error: %s", msg);
    }

    return 0;
}


/*
 *  Control whether the HTTP connection is kept alive after this request
 */
static EjsVar *keepAlive(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  Load a view. 
 *
 *  NOTE: The view name is not the file name nor is it the View class name. Rather it is a component of the
 *  class name. For example, in a stand-alone file with a .ejs extension, the view name will be StandAlone and the class
 *  name will be: Application_StandAloneView. For an application view, the name will be the name of a view within the 
 *  scope of a controller. For example: in a controller called "Portfolio", there may be a view called "edit". The resulting
 *  view class would be "Portfolio_editView"
 *
 *  If the view argument is not supplied, use the action name.
 *
 *  loadView(viewName: String = null)
 */
static EjsVar *loadView(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    EjsError    *e;
    int         rc;

    web = ejsGetHandle(ejs);
    if (argc > 0) {
        web->viewName = ejsGetString(argv[0]);
    }
    rc = ejsLoadView(ejs);
    if (rc < 0) {
        e = (EjsError*) ejsThrowError(ejs, "%s", web->error ? web->error : "Can't load view");
        if (ejsIsError(e)) {
            e->code = (rc == MPR_ERR_NOT_FOUND) ? MPR_HTTP_CODE_NOT_FOUND : MPR_HTTP_CODE_INTERNAL_SERVER_ERROR;
        }
        return 0;
    }
    return 0;
}


/*
 *  Redirect the client's browser to a new URL.
 *
 *  function redirectUrl(url: String, code: Number = 302): Void
 */
static EjsVar *redirectUrl(Ejs *ejs, EjsVar *controller, int argc, EjsVar **argv)
{
    char        *url;
    int         code;

    mprAssert(argc == 1 || argc == 2);

    url = ejsGetString(argv[0]);
    code = (argc == 2) ? ejsGetInt(argv[1]): 302;

    ejsRedirect(ejs, code, url);

    ejsSetProperty(ejs, controller, ES_ejs_web_Controller_rendered, (EjsVar*) ejs->trueValue);

    return 0;
}


/*
 *  Set a response header.
 *
 *  function setHeader(key: String, value: String, allowMultiple: Boolean = true): Void
 */
static EjsVar *setHeader(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    char        *key, *value;
    bool        allowMultiple;

    mprAssert(argc == 1 || argc == 2);

    web = ejsGetHandle(ejs);
    key = ejsGetString(argv[0]);
    value = ejsGetString(argv[1]);
    allowMultiple = (argc == 3) ? ejsGetBoolean(argv[2]) : 1;

    web->control->setHeader(web->handle, allowMultiple, key, value);
    return 0;
}


/**
 *  Define a cookie header to include in the reponse
 
 *  native function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void
 */
static EjsVar *setCookie(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    char        *name, *value, *path;
    int         lifetime, secure;
    
    mprAssert(4 <= argc && argc <= 5);
    web = ejsGetHandle(ejs);

    name = ejsGetString(argv[0]);
    value = ejsGetString(argv[1]);
    lifetime = ejsGetInt(argv[2]);
    path = ejsGetString(argv[3]);
    secure = (argc == 5) ? ejsGetBoolean(argv[4]): 0;
    web->control->setCookie(web->handle, name, value, lifetime, path, secure);
    return 0;
}


/*
 *  Set the HTTP response code
 *
 *  native function setHttpCode(code: Number): Void
 */
static EjsVar *setHttpCode(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    int         code;
    
    mprAssert(argc == 1);
    web = ejsGetHandle(ejs);

    code = ejsGetInt(argv[0]);

    web->control->setHttpCode(web->handle, code);
    return 0;
}


/*
 *  Set the HTTP response code
 *
 *  native function setMimeType(format: String): Void
 */
static EjsVar *setMimeType(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    char        *mimeType;
    
    mprAssert(argc == 1);
    mprAssert(ejsIsString(argv[0]));

    web = ejsGetHandle(ejs);
    mimeType = ejsGetString(argv[0]);

    web->control->setMimeType(web->handle, mimeType);
    return 0;
}


/*
 *  Write text to the client. This call writes the arguments back to the client's browser. The arguments are converted
 *  to strings before writing back to the client. Text written using write, will be buffered by the web server.
 *  This allows text to be written prior to setting HTTP headers with setHeader.
 *
 *  function write(...args): Void
 */
static EjsVar *writeMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString       *s;
    EjsVar          *args, *vp;
    EjsByteArray    *ba;
    int             err, i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    err = 0;
    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp) {
            switch (vp->type->id) {
            case ES_String:
                s = (EjsString*) vp;
                if (ejsWriteBlock(ejs, s->value, s->length) != s->length) {
                    err++;
                }
                break;

            case ES_ByteArray:
                ba = (EjsByteArray*) vp;
                if (ejsWriteBlock(ejs, (char*) ba->value, ba->length) != ba->length) {
                    err++;
                }

            default:
                s = (EjsString*) ejsToString(ejs, vp);
                if (s) {
                    if (ejsWriteBlock(ejs, s->value, s->length) != s->length) {
                        err++;
                    }
                }
            }
            if (ejs->exception) {
                return 0;
            }
        }
    }
#if UNUSED
    if (ejsWriteString(ejs, "\r\n") != 2) {
        err++;
    }
#endif
    if (err) {
        ejsThrowIOError(ejs, "Can't write to browser");
    }

    return 0;
}


/*
 *  The controller type is a scripted class augmented by native methods.
 */
void ejsConfigureWebControllerType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Controller"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find ejs.web Controller class");
            ejs->hasError = 1;
        }
        return;
    }

    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_cache, (EjsNativeFunction) cache);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_createSession, (EjsNativeFunction) createSession);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_destroySession, (EjsNativeFunction) destroySession);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_discardOutput, (EjsNativeFunction) discardOutput);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_sendError, (EjsNativeFunction) sendError);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_keepAlive, (EjsNativeFunction) keepAlive);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_loadView, (EjsNativeFunction) loadView);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_redirectUrl, (EjsNativeFunction) redirectUrl);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setCookie, (EjsNativeFunction) setCookie);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setHeader, (EjsNativeFunction) setHeader);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setHttpCode, (EjsNativeFunction) setHttpCode);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setMimeType, (EjsNativeFunction) setMimeType);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_write, (EjsNativeFunction) writeMethod);
}

#endif /* BLD_FEATURE_EJS_WEB */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebController.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebFramework.c"
 */
/************************************************************************/

/*
 *  ejsWebFramework.c -- Ejscript web framework processing.
 *
 *  Ejscript provides an MVC paradigm for efficiently creating dynamic applications using server-side Javascript.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



#if BLD_FEATURE_EJS_WEB

/*
 *  Singleton instance for the Web server control structure
 */
static EjsWebControl *webControl;


static int  compile(EjsWeb *web, cchar *kind, cchar *name);
static void createCookie(Ejs *ejs, EjsVar *cookies, cchar *name, cchar *value, cchar *domain, cchar *path);
static int  initInterp(Ejs *ejs, EjsWebControl *control);
static int  loadApplication(EjsWeb *web);
static int  loadController(EjsWeb *web);
static int  loadComponent(EjsWeb *web, cchar *kind, cchar *name, cchar *ext);
static int  build(EjsWeb *web, cchar *kind, cchar *name, cchar *base, cchar *ext);
static int  parseControllerAction(EjsWeb *web);

/*
 *  Create and configure web framework types
 */
void ejsConfigureWebTypes(Ejs *ejs)
{
    ejsConfigureWebRequestType(ejs);
    ejsConfigureWebResponseType(ejs);
    ejsConfigureWebHostType(ejs);
    ejsConfigureWebControllerType(ejs);
    ejsConfigureWebSessionType(ejs);
}


/*
 *  Loadable module interface. Called when loaded from a shared library.
 */
MprModule *ejs_webModuleInit(Ejs *ejs)
{
    MprModule   *module;

    module = mprCreateModule(ejs, "ejsWeb", BLD_VERSION, 0, 0, 0);
    if (module == 0) {
        return 0;
    }

    ejsSetGeneration(ejs, EJS_GEN_ETERNAL);
    ejsConfigureWebTypes(ejs);
    ejsSetGeneration(ejs, EJS_GEN_OLD);

    return module;
}


/*
 *  Called once by the web server handler when it it loaded.
 */
int ejsOpenWebFramework(EjsWebControl *control, bool useMaster)
{
    mprAssert(control);

    /*
     *  Create the Ejscript service
     */
    control->service = ejsCreateService(control);
    if (control->service == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    if (useMaster) {
        /*
         *  Create the master interpreter
         */
        control->master = ejsCreate(control->service, 0, EJS_FLAG_MASTER);
        if (control->master == 0) {
            mprAssert(control->master);
            mprFree(control->service);
            return MPR_ERR_NO_MEMORY;
        }

        if (initInterp(control->master, control) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    webControl = control;
    return 0;
}


static int initInterp(Ejs *ejs, EjsWebControl *control)
{
    EjsVar      *sessions;

#if !BLD_FEATURE_STATIC
    if (ejsLoadModule(ejs, "ejs.web", NULL, NULL, 0) == 0) {
        mprError(control, "Can't load ejs.web.mod: %s", ejsGetErrorMsg(ejs, 1));
        return MPR_ERR_CANT_INITIALIZE;;
    }
#endif

#if FUTURE
    control->applications = ejsCreateSimpleObject(ejs);
#endif

    //  TODO - should session timeouts per different per app?
    control->sessionTimeout = EJS_SESSION_TIMEOUT;
#if ES_ejs_web_sessions
    sessions = ejsGetProperty(ejs, ejs->global, ES_ejs_web_sessions);
#else
{
    EjsName qname;
    sessions = ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "sessions"));
}
#endif
    ejsMakePermanent(ejs, sessions);
    control->sessions = (EjsObject*) sessions;

    return 0;
}


/*
 *  Return the web server master interpreter. If not using a master interpreter, it returns the current interp.
 */
Ejs *ejsGetMaster(MprCtx ctx)
{
    return (Ejs*) webControl->master;
}


/*
 *  Initialize a new web request structure. 
 *  Given request       "/carmen/admin/list/1?a=b&c=d", then the args would be:
 *      baseUrl         "/carmen"
 *      url             "/admin/list/1"
 *      query           "a=b&c=d"
 *      baseDir         "/path/carmen"
 */
EjsWeb *ejsCreateWebRequest(MprCtx ctx, EjsWebControl *control, void *handle, cchar *baseUrl, cchar *url,
        cchar *baseDir, int flags)
{
    Ejs             *ejs;
    EjsWeb          *web;
    cchar           *appUrl;

    web = (EjsWeb*) mprAllocObjZeroed(ctx, EjsWeb);
    if (web == 0) {
        return 0;
    }

    if (flags & EJS_WEB_FLAG_APP) {
        appUrl = baseUrl;

    } else {
        appUrl = 0;
        flags |= EJS_WEB_FLAG_SOLO;
    }

    web->appDir = mprStrdup(web, baseDir);
    mprStrTrim((char*) web->appDir, "/");
    web->appUrl = appUrl;
    web->url = url;

    web->flags = flags;
    web->handle = handle;
    web->control = control;

    if (control->master) {
        ejs = web->ejs = ejsCreate(ctx, control->master, 0);
        ejs->master = control->master;
    } else {
        ejs = web->ejs = ejsCreate(ctx, 0, 0);
        if (ejs) {
            if (initInterp(ejs, control) < 0) {
                mprFree(web);
                return 0;
            }
        }
    }
    if (ejs == 0) {
        mprFree(web);
        return 0;
    }

    ejsSetHandle(ejs, web);

    //  TODO - temp
    if (control->master) {
        control->master->gc.enabled = 0;
    }
    ejs->gc.enabled = 0;

    mprLog(ctx, 3, "EJS: new request: AppDir %s, AppUrl %s, URL %s", web->appDir, web->appUrl, web->url);

    return web;
}


/*
 *  Parse the request URI and create the controller and action names. URI is in the form: "controller/action"
 */
static int parseControllerAction(EjsWeb *web)
{
    cchar   *url;
    char    *cp, *controllerName;

    if (web->flags & EJS_WEB_FLAG_SOLO || strstr(web->url, EJS_WEB_EXT)) {
        if (web->flags & EJS_WEB_FLAG_SOLO) {
            ejsName(&web->controllerName, "ejs.web", "_SoloController");
        } else {
            ejsName(&web->controllerName, EJS_PUBLIC_NAMESPACE, "BaseController");
        }
        ejsName(&web->doActionName, "ejs.web", "renderView");

        /*
         *  View name strips extension and converts "/" to "_"
         */
        web->viewName = mprStrdup(web, &web->url[1]);
        if ((cp = strchr(web->viewName, '.')) != 0) {
            *cp = '\0';
        }
        for (cp = web->viewName; *cp; cp++) {
            if (*cp == '/') {
                *cp = '_';
            }
        }
        return 0;
    }

    /*
     *  Request as part of an Ejscript application (not stand-alone)
     */
    for (url = web->url; *url == '/'; url++) {
        ;
    }
    controllerName = mprStrdup(web, url);
    controllerName[0] = toupper((int) controllerName[0]);
    mprStrTrim(controllerName, "/");

    web->viewName = "";
    if ((cp = strchr(controllerName, '/')) != 0) {
        *cp++ = '\0';
        web->viewName = cp;
        if ((cp = strchr(cp, '/')) != 0) {
            *cp++ = '\0';
        }
    }
    if (*controllerName == '\0') {
        controllerName = "Base";
    }
    if (*web->viewName == '\0') {
        web->viewName = "index";
    }

    mprAllocSprintf(web, &cp, -1, "%sController", controllerName);
    ejsName(&web->controllerName, EJS_PUBLIC_NAMESPACE, cp);
    web->controllerFile = controllerName;

    ejsName(&web->doActionName, "ejs.web", "doAction");

    return 0;
}


static int loadApplication(EjsWeb *web)
{
    return loadComponent(web, "app", "App", ".es");
}


/*
 *  Load the controller module
 */
static int loadController(EjsWeb *web)
{
    return loadComponent(web, "controller", web->controllerFile, ".es");
}


static int createController(EjsWeb *web)
{
    Ejs         *ejs;
    EjsVar      *host, *request, *response, *argv[16];
    EjsType     *type;
    int         oldGen, slotNum;

    ejs = web->ejs;

    if (web->flags & EJS_WEB_FLAG_APP) {
        if (loadApplication(web) < 0) {
            return MPR_ERR_NOT_FOUND;
        }
    }

    web->controllerType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &web->controllerName);
    if (web->controllerType == 0 || !ejsIsType(web->controllerType)) {
        if (web->controllerFile && loadController(web) < 0) {
            return MPR_ERR_NOT_FOUND;
        }
        web->controllerType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &web->controllerName);
        if (web->controllerType == 0 || !ejsIsType(web->controllerType)) {
            mprAllocSprintf(web, &web->error, -1, "Can't find controller %s", web->controllerName.name);
            return MPR_ERR_NOT_FOUND;
        }
    }

    oldGen = ejsSetGeneration(ejs, EJS_GEN_ETERNAL);

    if ((web->cookie = (char*) ejsGetHeader(ejs, "HTTP_COOKIE")) != 0) {
        ejsParseWebSessionCookie(web);
    }

    if (web->flags & EJS_WEB_FLAG_SESSION && web->session == 0) {
        //  TODO - not setting secure appropriately. Last parameter is "secure"
        web->session = ejsCreateSession(ejs, 0, 0);
    }

    /*
     *  Create the Host, Request and Response objects. These are virtual objects that lazily create their properties.
     */
    host =      (EjsVar*) ejsCreateWebHostObject(ejs, web->handle);
    response =  (EjsVar*) ejsCreateWebResponseObject(ejs, web->handle);
    request =   (EjsVar*) ejsCreateWebRequestObject(ejs, web->handle);

    argv[0] = (web->flags & EJS_WEB_FLAG_SOLO) ? (EjsVar*) ejs->falseValue : (EjsVar*) ejs->trueValue;
    argv[1] = (EjsVar*) ejsCreateString(ejs, web->appDir);
    argv[2] = (EjsVar*) ejsCreateString(ejs, web->appUrl);
    argv[3] = (EjsVar*) web->session;
    argv[4] = host;
    argv[5] = request;
    argv[6] = response;

    web->controller = (EjsVar*) ejsCreateObject(ejs, web->controllerType, 0);
    if (web->controller == 0) {
        /* TODO - functionalize this */
        web->error = "Memory allocation failure";
        return MPR_ERR_NO_MEMORY;
    }

    ejsRunFunctionBySlot(ejs, web->controller, ES_ejs_web_Controller_ejs_web_initialize, 7, argv);

    type = (EjsType*) web->controllerType;
    if (type->hasConstructor) {
        slotNum = type->block.numInherited;
        ejsRunFunctionBySlot(ejs, web->controller, slotNum, 0, NULL);
    }

    web->params = ejsGetProperty(ejs, web->controller, ES_ejs_web_Controller_params);
    //  TODO - rc
    ejsDefineParams(ejs);
    ejsSetGeneration(ejs, oldGen);
    
    return 0;
}


static int getDoAction(EjsWeb *web)
{
    Ejs     *ejs;

    ejs = web->ejs;
    web->doAction = ejsGetPropertyByName(ejs, (EjsVar*) web->controllerType, &web->doActionName);
    if (web->doAction == 0 || !ejsIsFunction(web->doAction)) {
        mprAllocSprintf(web, &web->error, -1, "Internal error: Can't find function %s::%s", web->doActionName.space,
                web->doActionName.name);
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Run a web request
 */
int ejsRunWebRequest(EjsWeb *web)
{
    Ejs             *ejs;
    EjsVar          *result, *argv[1];

    ejs = web->ejs;

    /*
     *  Parse the url and extract the controller and action name
     */
    if (parseControllerAction(web) < 0) {
        mprAllocSprintf(web, &web->error, -1, "URL is not in the right form: \"%s\"", web->url);
        return MPR_ERR_BAD_ARGS;
    }
    if (createController(web) < 0) {
        return MPR_ERR_CANT_CREATE;
    }
    if (getDoAction(web) < 0) {
        return MPR_ERR_CANT_CREATE;
    }

    argv[0] = (EjsVar*) ejsCreateString(ejs, web->viewName);
    result = ejsRunFunction(ejs, (EjsFunction*) web->doAction, web->controller, 1, argv);
    if (result == 0 && ejs->exception) {
        web->error = ejsGetErrorMsg(ejs, 1);
        return MPR_ERR_CANT_COMPLETE;
    }
    return 0;
}


int ejsLoadView(Ejs *ejs)
{
    EjsWeb      *web;
    char        name[MPR_MAX_FNAME], *cp;

    mprAssert(ejs);
    web = ejs->handle;

    if (!(web->flags & EJS_WEB_FLAG_SOLO) && !strstr(web->url, EJS_WEB_EXT)) {
        /*
         *  Normal views/...
         */
        mprSprintf(name, sizeof(name), "%s/%s", web->controllerFile, web->viewName);
        return loadComponent(web, "view", name, EJS_WEB_EXT);

    }
    mprStrcpy(name, sizeof(name), &web->url[1]);
    if ((cp = strrchr(name, '.')) && strcmp(cp, EJS_WEB_EXT) == 0) {
        *cp = '\0';
    }
    return loadComponent(web, "", name, EJS_WEB_EXT);
}


/*
 *  Load a module corresponding to a source component. If the source is newer, then recompile the component. Build the 
 *  name to the module from web->appdir and name depending on the kind. Kind will be "app", "controller", "view" or "".
 */
static int loadComponent(EjsWeb *web, cchar *kind, cchar *name, cchar *ext)
{
    Ejs         *ejs;
    char        base[MPR_MAX_FNAME], nameBuf[MPR_MAX_FNAME], *delim;
    int         rc;

    ejs = web->ejs;

    delim = (web->appDir[strlen(web->appDir) - 1] == '/') ? "" : "/";

    if (strcmp(kind, "app") == 0) {
        mprSprintf(base, sizeof(base), "%s%s%s", web->appDir, delim, name);
        //  TODO - need to recompile the app with models somehow

    } else if (*kind) {
        /* Note we pluralize the kind (e.g. view to views) */
        mprSprintf(base, sizeof(base), "%s%s%ss/%s", web->appDir, delim, kind, name);
        if ((rc = build(web, kind, name, base, ext)) < 0) {
            return rc;
        }

    } else {
        /*
         *  Solo web pages
         */
        mprSprintf(base, sizeof(base), "%s%s%s", web->appDir, delim, name);
        mprSprintf(nameBuf, sizeof(nameBuf), "%s%s", name, ext);
        name = nameBuf;
        if ((rc = build(web, kind, name, base, ext)) < 0) {
            return rc;
        }
    }

    if (ejsLoadModule(web->ejs, base, NULL, NULL, 0) == 0) {
        mprAllocSprintf(web, &web->error, -1, "Can't load module : \"%s.mod\"\n%s", base, ejsGetErrorMsg(ejs, 1));
        return MPR_ERR_CANT_READ;
    }
    return 0;
}


/*
 *  Compile a component into a loadable module. Return true if the compile succeeded.
 */
static int compile(EjsWeb *web, cchar *kind, cchar *name)
{
    MprCmd      *cmd;
    cchar       *dir;
    char        target[MPR_MAX_FNAME], appDir[MPR_MAX_FNAME], commandLine[MPR_MAX_FNAME * 4];
    char        *path, *err;
    int         status;

    if (strcmp(kind, "view") == 0) {
        mprSprintf(target, sizeof(target), "views/%s", name);
        name = target;
    }

    cmd = mprCreateCmd(web);
    mprSetCmdDir(cmd, web->appDir);

#if FUTURE
    /*
     *  Enable this when supported external Ejscript modules for Appweb
     */
    if (web->control->modulePath && *web->control->modulePath) {
        /*
         *  Location where the Ejscript module was loaded from (typically BLD_MOD_PREFIX)
         */
        dir = web->control->modulePath;
        /*
         *  Add this below to the search path when enabled
         */
        dir, "/../../bin", MPR_SEARCH_DELIM,    //  Search relative to the module path for dev trees
    } else {
#endif

    dir = mprGetAppDir(web, appDir, sizeof(appDir));

    /*
     *  Search for ejsweb
     */
    path = mprSearchForFile(web, EJS_EJSWEB_EXE, MPR_SEARCH_EXE, 
        dir, MPR_SEARCH_DELIM,                  //  Search in same dir as application (or override module path) (Windows)
        BLD_BIN_PREFIX, MPR_SEARCH_DELIM,       //  Search the standard binary install directory
        BLD_ABS_BIN_DIR, NULL);                 //  Search the local dev bin
	if (path == 0) {
        mprError(web, "Can't find %s program", EJS_EJSWEB_EXE);
        return MPR_ERR_CANT_ACCESS;
    }

    mprSprintf(commandLine, sizeof(commandLine), "\"%s\" --quiet compile %s \"%s\"", path, kind, name);
    mprLog(web, 4, "Running %s", commandLine);

    status = mprRunCmd(cmd, commandLine, NULL, &err, 0);
    if (status) {
        web->error = mprStrdup(web, err);
        mprLog(web, 3, "Compilation failure for %s\n%s", commandLine, err);
    }
    mprFree(cmd);
    return status;
}


/*
 *  Buidl a resource
 */
static int build(EjsWeb *web, cchar *kind, cchar *name, cchar *base, cchar *ext)
{
    MprFileInfo     moduleInfo, sourceInfo;
    char            module[MPR_MAX_FNAME], source[MPR_MAX_FNAME];

    mprSprintf(module, sizeof(module), "%s.mod", base);
    mprSprintf(source, sizeof(source), "%s%s", base, ext);
    mprGetFileInfo(web, module, &moduleInfo);
    mprGetFileInfo(web, source, &sourceInfo);

    if (!sourceInfo.valid) {
        mprLog(web, 3, "Can't find resource %s", source);
        mprAllocSprintf(web, &web->error, -1, "Can't find resource: \"%s\"", source);
        return MPR_ERR_NOT_FOUND;
    }
    if (moduleInfo.valid && sourceInfo.valid && sourceInfo.mtime < moduleInfo.mtime) {
        /* Up to date already */
        mprLog(web, 5, "Resource %s is up to date", source);
        return 0;
    }
    if (compile(web, kind, name) != 0) {
        return MPR_ERR_BAD_STATE;
    }
    return 0;
}


/*
 *  This routine parses the cookie header to search for a session cookie.
 *  There may be multiple cookies where the most qualified path come first
 */
EjsVar *ejsCreateCookies(Ejs *ejs)
{
    EjsWeb      *web;
    cchar       *domain, *path, *version, *cookieName, *cookieValue;
    char        *cookieString, *value, *tok, *key, *dp, *sp;
    int         seenSemi;

    web = ejs->handle;
    if (web->cookies) {
        return web->cookies;
    }

    web->cookies = (EjsVar*) ejsCreateSimpleObject(ejs);

    cookieString = mprStrdup(web, web->cookie);
    key = cookieString;
    cookieName = cookieValue = domain = path = 0;

    while (*key) {
        while (*key && isspace(*key)) {
            key++;
        }
        tok = key;
        while (*tok && !isspace(*tok) && *tok != ';' && *tok != '=') {
            tok++;
        }
        if (*tok) {
            *tok++ = '\0';
        }
        while (isspace(*tok)) {
            tok++;
        }

        seenSemi = 0;
        if (*tok == '\"') {
            value = ++tok;
            while (*tok != '\"' && *tok != '\0') {
                tok++;
            }
            if (*tok) {
                *tok++ = '\0';
            }

        } else {
            value = tok;
            while (*tok != ';' && *tok != '\0') {
                tok++;
            }
            if (*tok) {
                seenSemi++;
                *tok++ = '\0';
            }
        }

        /*
         *  Handle back-quoting in value
         */
        if (strchr(value, '\\')) {
            for (dp = sp = value; *sp; sp++) {
                if (*sp == '\\') {
                    sp++;
                }
                *dp++ = *sp++;
            }
            *dp = '\0';
        }

        /*
         *  Example:
         *  $Version="1"; NAME="value"; $Path="PATH"; $Domain="DOMAIN"; NAME="value"; $Path="PATH"; $Domain="DOMAIN"; 
         */
        if (*key == '$') {
            key++;
            switch (tolower(*key)) {
            case 'd':
                if (mprStrcmpAnyCase(key, "domain") == 0) {
                    domain = value;
                }
                break;

            case 'p':
                if (mprStrcmpAnyCase(key, "path") == 0) {
                    path = value;
                }
                break;

            case 'v':
                if (mprStrcmpAnyCase(key, "version") == 0) {
                    version = value;
                }
                break;
            default:
                break;
            }
            
        } else {
            if (cookieName) {
                createCookie(ejs, web->cookies, cookieName, cookieValue, domain, path);
                cookieName = cookieValue = path = domain = 0;
            }
            cookieName = key;
            cookieValue = value;
        }

        key = tok;
        if (!seenSemi) {
            while (*key && *key != ';') {
                key++;
            }
            if (*key) {
                key++;
            }
        }
    }
    if (cookieName) {
        createCookie(ejs, web->cookies, cookieName, cookieValue, domain, path);
    }
    mprFree(cookieString);
    return web->cookies;
}


static EjsVar *createString(Ejs *ejs, cchar *value)
{
    return value ? (EjsVar*) ejsCreateString(ejs, value) : ejs->nullValue;
}


static void createCookie(Ejs *ejs, EjsVar *cookies, cchar *name, cchar *value, cchar *domain, cchar *path)
{
    EjsType     *cookieType;
    EjsName     qname;
    EjsVar      *cookie;
    
#if ES_ejs_web_Cookie
    cookieType = ejsGetType(ejs, ES_ejs_web_Cookie);
#else
    cookieType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Cookie"));
#endif
    mprAssert(cookieType);

    cookie = (EjsVar*) ejsCreateObject(ejs, cookieType, 0);

    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_name, createString(ejs, name));
    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_value, createString(ejs, value));
    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_path, createString(ejs, path));
    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_domain, createString(ejs, domain));

    ejsSetPropertyByName(ejs, cookies, ejsName(&qname, "", name), (EjsVar*) cookie);
}


/*
 *  Define a form variable as an ejs property in the params[] collection. Support a.b.c syntax
 */
void ejsDefineWebParam(Ejs *ejs, cchar *key, cchar *value)
{
    EjsName     qname;
    EjsWeb      *web;
    EjsVar      *where, *vp;
    char        *subkey, *end;
    int         slotNum;

    web = ejsGetHandle(ejs);

    where = web->params;
    mprAssert(where);

    /*
     *  name.name.name
     */
    if (strchr(key, '.') == 0) {
        ejsName(&qname, "", key);
        ejsSetPropertyByName(ejs, where, &qname, (EjsVar*) ejsCreateString(ejs, value));

    } else {
        key = subkey = mprStrdup(ejs, key);
        for (end = strchr(subkey, '.'); end; subkey = end, end = strchr(subkey, '.')) {
            *end++ = '\0';
            ejsName(&qname, "", subkey);
            vp = ejsGetPropertyByName(ejs, where, &qname);
            if (vp == 0) {
                slotNum = ejsSetPropertyByName(ejs, where, &qname, (EjsVar*) ejsCreateObject(ejs, ejs->objectType, 0));
                vp = ejsGetProperty(ejs, where, slotNum);
            }
            where = vp;
        }
        mprAssert(where);
        ejsName(&qname, "", subkey);
        ejsSetPropertyByName(ejs, where, &qname, (EjsVar*) ejsCreateString(ejs, value));
        mprFree((char*) key);
    }
}


#endif /* BLD_FEATURE_EJS_WEB */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebFramework.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebHost.c"
 */
/************************************************************************/

/**
 *  ejsWebHost.c - Native code for the Host class.
 *
 *  The Host properties are "virtual" in that they are created lazily and don't really exist in this class. Rather, 
 *  they are accessed from the web server as required.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_WEB

/*
 *  If shared, there will not be a global slot defined for Host
 */
#ifndef ES_ejs_web_Host
#define ES_ejs_web_Host -1
#endif

/*
 *  Lookup a property. These properties are virtualized.
 */
static EjsVar *getHostProperty(Ejs *ejs, EjsWebHost *rq, int slotNum)
{
    return ejsGetWebVar(ejs, EJS_WEB_HOST_VAR, slotNum);
}


static int getHostPropertyCount(Ejs *ejs, EjsWebHost *rq)
{
    return ES_ejs_web_Host_NUM_INSTANCE_PROP;
}


static EjsName getHostPropertyName(Ejs *ejs, EjsWebHost *rq, int slotNum)
{
    return ejsGetPropertyName(ejs, (EjsVar*) rq->var.type->instanceBlock, slotNum);
}


/*
 *  Lookup a property by name.
 */
static int lookupHostProperty(struct Ejs *ejs, EjsWebHost *rq, EjsName *qname)
{
    return ejsLookupProperty(ejs, (EjsVar*) rq->var.type->instanceBlock, qname);
}


/*
 *  Update a property's value.
 */
static int setHostProperty(struct Ejs *ejs, EjsWebHost *ap, int slotNum,  EjsVar *value)
{
    return ejsSetWebVar(ejs, EJS_WEB_HOST_VAR, slotNum, value);
}



EjsWebHost *ejsCreateWebHostObject(Ejs *ejs, void *handle)
{
    EjsWebHost  *vp;
    EjsType     *requestType;
    EjsName     qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Host"));
    vp = (EjsWebHost*) ejsCreateVar(ejs, requestType, 0);

    ejsSetDebugName(vp, "EjsWeb Host Instance");
    return vp;
}


void ejsConfigureWebHostType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Host"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Host class");
            ejs->hasError = 1;
        }
        return;
    }
    
    type->instanceSize = sizeof(EjsWebHost);
    type->hasObject = 0;

    /*
     *  Define the helper functions.
     */
    *type->helpers = *ejs->defaultHelpers;
    type->helpers->getProperty = (EjsGetPropertyHelper) getHostProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getHostPropertyCount;
    type->helpers->getPropertyName = (EjsGetPropertyNameHelper) getHostPropertyName;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupHostProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setHostProperty;
}

#endif /* BLD_FEATURE_EJS_WEB */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebHost.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebInterface.c"
 */
/************************************************************************/

/*
 *  ejsWebInterface.c - Wrappers for the web framework control block functions.
 *
 *  These functions provide the API for web servers to invoke.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



void ejsDefineParams(Ejs *ejs)
{
    EjsWeb         *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);

    if (web->control->defineParams) {
        web->control->defineParams(web->handle);
    }
}


/*
 *  Discard all generated output
 */
void ejsDiscardOutput(Ejs *ejs)
{
    EjsWeb         *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);

    if (web->control->discardOutput) {
        web->control->discardOutput(web->handle);
    }
}


/*
 *  Return an error to the client
 */
void ejsWebError(Ejs *ejs, int code, cchar *fmt, ...)
{
    EjsWeb      *web;
    va_list     args;
    char        *buf;

    mprAssert(ejs);
    mprAssert(fmt);

    va_start(args, fmt);
    mprAllocVsprintf(ejs, &buf, -1, fmt, args);
    web = ejsGetHandle(ejs);

    if (web->control->error) {
        web->control->error(web->handle, code, buf);
    }
    mprFree(buf);
    va_end(args);
}


/*
 *  Get a HTTP request header
 */
cchar *ejsGetHeader(Ejs *ejs, cchar *key)
{
    EjsWeb     *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);
    mprAssert(web->control->getHeader);
    return web->control->getHeader(web->handle, key);
}


/*
 *  Get a variable from the web server. This is used to implement virtual properties
 */
EjsVar *ejsGetWebVar(Ejs *ejs, int collection, int field)
{
    EjsWeb     *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);
    mprAssert(web->control->getVar);
    return web->control->getVar(web->handle, collection, field);
}


#if UNUSED
int ejsMapToStorage(Ejs *ejs, char *path, int pathsize, cchar *url)
{
    EjsWeb     *web;

    //  TODO - include files are not required anymore
    mprAssert(0);
    mprAssert(ejs);
    web = ejsGetHandle(ejs);
    mprAssert(web->control->mapToStorage);
    return web->control->mapToStorage(web->handle, path, pathsize, url);
}


int ejsReadFile(Ejs *ejs, char **buf, int *len, cchar *path)
{
    EjsWeb     *web;

    //  TODO - what is this for?
    mprAssert(0);
    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->readFile);
    return web->control->readFile(web->handle, buf, len, path);
}
#endif


void ejsRedirect(Ejs *ejs, int code, cchar *url)
{
    EjsWeb     *web;

    mprAssert(ejs);
    mprAssert(url);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->redirect);
    web->control->redirect(web->handle, code, url);
}


void ejsSetCookie(Ejs *ejs, cchar *name, cchar *value, int lifetime, cchar *path, bool secure)
{
    EjsWeb     *web;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->setCookie);
    web->control->setCookie(web->handle, name, value, lifetime, path, secure);
}


void ejsSetWebHeader(Ejs *ejs, bool allowMultiple, cchar *key, cchar *fmt, ...)
{
    EjsWeb     *web;
    char            *value;
    va_list         vargs;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->setHeader);

    va_start(vargs, fmt);
    mprAllocVsprintf(web, &value, -1, fmt, vargs);

    web->control->setHeader(web->handle, allowMultiple, key, value);
}


void ejsSetHttpCode(Ejs *ejs, int code)
{
    EjsWeb     *web;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->setHttpCode);
    web->control->setHttpCode(web->handle, code);
}


/*
 *  Set a variable in the web server. This is used to implement virtual properties
 */
int ejsSetWebVar(Ejs *ejs, int collection, int field, EjsVar *value)
{
    EjsWeb     *web;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    if (web->control->setVar == 0) {
        ejsThrowReferenceError(ejs, "Object is read-only");
        return EJS_ERR;
    }
    mprAssert(web->control->setVar);
    return web->control->setVar(web->handle, collection, field, value);
}


int ejsWriteBlock(Ejs *ejs, cchar *buf, int size)
{
    EjsWeb     *web;

    mprAssert(ejs);
    mprAssert(buf);
    mprAssert(size >= 0);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->write);
    return web->control->write(web->handle, buf, size);
}


int ejsWriteString(Ejs *ejs, cchar *buf)
{
    EjsWeb     *web;

    mprAssert(ejs);
    mprAssert(buf);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->write);
    return web->control->write(web->handle, buf, (int) strlen(buf));
}


int ejsWrite(Ejs *ejs, cchar *fmt, ...)
{
    EjsWeb     *web;
    va_list     args;
    char        *buf;
    int         rc, len;

    mprAssert(ejs);
    mprAssert(fmt);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->write);

    va_start(args, fmt);
    len = mprAllocVsprintf(web, &buf, -1, fmt, args);
    rc = web->control->write(web->handle, buf, len);
    mprFree(buf);
    va_end(args);

    return rc;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebInterface.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebRequest.c"
 */
/************************************************************************/

/**
 *  ejsWebRequest.c - Native code for the Request class.
 *
 *  The Request properties are "virtual" in that they are created lazily and don't really exist in this class. Rather, 
 *  they are accessed from the web server as required.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_WEB

/*
 *  Lookup a property. These properties are virtualized.
 */
static EjsVar *getRequestProperty(Ejs *ejs, EjsWebRequest *rq, int slotNum)
{
    return ejsGetWebVar(ejs, EJS_WEB_REQUEST_VAR, slotNum);
}


static int getRequestPropertyCount(Ejs *ejs, EjsWebRequest *rq)
{
    return ES_ejs_web_Request_NUM_INSTANCE_PROP;
}


static EjsName getRequestPropertyName(Ejs *ejs, EjsWebRequest *rq, int slotNum)
{
    return ejsGetPropertyName(ejs, (EjsVar*) rq->var.type->instanceBlock, slotNum);
}


static int lookupRequestProperty(Ejs *ejs, EjsWebRequest *rq, EjsName *qname)
{
    return ejsLookupProperty(ejs, (EjsVar*) rq->var.type->instanceBlock, qname);
}


static int setRequestProperty(Ejs *ejs, EjsWebRequest *rq, int slotNum,  EjsVar *value)
{
    ejsThrowReferenceError(ejs, "Property is readonly");
    return 0;
}



EjsWebRequest *ejsCreateWebRequestObject(Ejs *ejs, void *handle)
{
    EjsWebRequest   *vp;
    EjsType         *requestType;
    EjsName         qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Request"));
    vp = (EjsWebRequest*) ejsCreateVar(ejs, requestType, 0);
    ejsSetDebugName(vp, "EjsWeb Request Instance");

    return vp;
}


void ejsConfigureWebRequestType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Request"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Request class");
            ejs->hasError = 1;
        }
        return;
    }
    type->instanceSize = sizeof(EjsWebRequest);
    type->hasObject = 0;

    /*
     *  Re-define the helper functions.
     */
    *type->helpers = *ejs->defaultHelpers;
    type->helpers->getProperty = (EjsGetPropertyHelper) getRequestProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getRequestPropertyCount;
    type->helpers->getPropertyName = (EjsGetPropertyNameHelper) getRequestPropertyName;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupRequestProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setRequestProperty;
}

#endif /* BLD_FEATURE_EJS_WEB */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebRequest.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebResponse.c"
 */
/************************************************************************/

/**
 *  ejsWebResponse.c - Web framework Response class.
 *
 *  The Response properties are "virtual" in that they are created lazily and don't really exist in this class. Rather, 
 *  they are accessed from the web server as required.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_WEB

/*
 *  Lookup a property. These properties are virtualized.
 */
static EjsVar *getResponseProperty(Ejs *ejs, EjsWebResponse *rq, int slotNum)
{
    return ejsGetWebVar(ejs, EJS_WEB_RESPONSE_VAR, slotNum);
}


static int getResponsePropertyCount(Ejs *ejs, EjsWebResponse *rq)
{
    return ES_ejs_web_Response_NUM_INSTANCE_PROP;
}


static EjsName getResponsePropertyName(Ejs *ejs, EjsWebResponse *rq, int slotNum)
{
    return ejsGetPropertyName(ejs, (EjsVar*) rq->var.type->instanceBlock, slotNum);
}

/*
 *  Lookup a property by name.
 */
static int lookupResponseProperty(struct Ejs *ejs, EjsWebResponse *rq, EjsName *qname)
{
    return ejsLookupProperty(ejs, (EjsVar*) rq->var.type->instanceBlock, qname);
}


/*
 *  Update a property's value.
 */
static int setResponseProperty(struct Ejs *ejs, EjsWebResponse *ap, int slotNum,  EjsVar *value)
{
    return ejsSetWebVar(ejs, EJS_WEB_RESPONSE_VAR, slotNum, value);
}



EjsWebResponse *ejsCreateWebResponseObject(Ejs *ejs, void *handle)
{
    EjsWebResponse  *vp;
    EjsType         *requestType;
    EjsName         qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Response"));
    vp = (EjsWebResponse*) ejsCreateVar(ejs, requestType, 0);

    ejsSetDebugName(vp, "EjsWeb Response Instance");

    return vp;
}


void ejsConfigureWebResponseType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Response"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Response class");
            ejs->hasError = 1;
        }
        return;
    }
    type->instanceSize = sizeof(EjsWebResponse);
    type->hasObject = 0;

    /*
     *  Re-define the helper functions.
     */
    *type->helpers = *ejs->defaultHelpers;
    type->helpers->getProperty = (EjsGetPropertyHelper) getResponseProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getResponsePropertyCount;
    type->helpers->getPropertyName = (EjsGetPropertyNameHelper) getResponsePropertyName;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupResponseProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setResponseProperty;
}

#endif /* BLD_FEATURE_EJS_WEB */


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebResponse.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../web/ejsWebSession.c"
 */
/************************************************************************/

/**
 *  ejsWebSession.c - Native code for the Session class.
 *
 *  The Session class serializes objects that are stored to the session object.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_EJS_WEB

static void sessionActivity(Ejs *ejs, EjsWebSession *sp);
static void sessionTimer(EjsWebControl *control, MprEvent *event);

#if UNUSED
static void descape(EjsVar *vp)
{
    char    *str, *cp, *dp;
    
    if (!ejsIsString(vp)) {
        return;
    }
    
    str = ((EjsString*) vp)->value;
    
    if (str == 0 || *str == '\0') {
        return;
    }
    for (dp = cp = str; *cp; ) {
        if (*cp == '\\') {
            cp++;
        }
        *dp++ = *cp++;
    }
    *dp = '\0';
}
#endif


static EjsVar *getSessionProperty(Ejs *ejs, EjsWebSession *sp, int slotNum)
{
    EjsVar      *vp;
    EjsWeb      *web;

    web = ejs->handle;
    if (web->session != sp) {
        return (EjsVar*) ejs->emptyStringValue;
    }

    vp = ejs->objectHelpers->getProperty(ejs, (EjsVar*) sp, slotNum);
    if (vp) {
        vp = ejsDeserialize(ejs, vp);
#if UNUSED
        descape(vp);
#endif
    }
    if (vp == ejs->undefinedValue) {
        vp = (EjsVar*) ejs->emptyStringValue;
    }
    sessionActivity(ejs, sp);
    return vp;
}


static EjsVar *getSessionPropertyByName(Ejs *ejs, EjsWebSession *sp, EjsName *qname)
{
    EjsVar      *vp;
    EjsWeb      *web;
    int         slotNum;

    web = ejs->handle;
    if (web->session != sp) {
        return (EjsVar*) ejs->emptyStringValue;
    }

    qname->space = EJS_PUBLIC_NAMESPACE;
    slotNum = ejs->objectHelpers->lookupProperty(ejs, (EjsVar*) sp, qname);
    if (slotNum < 0) {
        /*
         *  Return empty string so that web pages can access session values without having to test for null/undefined
         */
        vp = (EjsVar*) ejs->emptyStringValue;
    } else {
        vp = ejs->objectHelpers->getProperty(ejs, (EjsVar*) sp, slotNum);
        if (vp) {
            vp = ejsDeserialize(ejs, vp);
#if UNUSED
            descape(vp);
#endif
        }
    }
    sessionActivity(ejs, sp);
    return vp;
}


#if UNUSED
static char *escape(MprCtx ctx, char *str)
{
    char    *cp, *bp, *buf;
    int     len;

    len = 0;
    for (cp = str; *cp; cp++) {
        if (*cp == '\'' || *cp == '"' || *cp == '\\') {
            len++;
        }
        len++;
    }

    bp = buf = mprAlloc(ctx, len + 1);
    if (bp == 0) {
        return 0;
    }

    for (bp = buf, cp = str; *cp; ) {
        if (*cp == '\'' || *cp == '"' || *cp == '\\') {
            *bp++ = '\\';
        }
        *bp++ = *cp++;
    }
    *bp = '\0';
    return buf;
}
#endif


static int setSessionProperty(Ejs *ejs, EjsWebSession *sp, int slotNum, EjsVar *value)
{
    EjsWeb  *web;
    
    web = ejs->handle;
    if (web->session != sp) {
        mprAssert(0);
        return EJS_ERR;
    }

    /*
     *  Allocate the serialized object using the master interpreter
     */
    if (ejs->master) {
        ejs = ejs->master;
    }

#if UNUSED
    if (value != ejs->undefinedValue && value != ejs->nullValue) {
        value = (EjsVar*) ejsSerialize(ejs, value, 0, 0, 0);
        estr = escape(ejs, ((EjsString*) value)->value);
        value = (EjsVar*) ejsCreateString(ejs, estr);
        mprFree(estr);
    }
#else
    value = (EjsVar*) ejsSerialize(ejs, value, 0, 0, 0);
#endif
    slotNum = ejs->objectHelpers->setProperty(ejs, (EjsVar*) sp, slotNum, value);
    
    sessionActivity(ejs, sp);
    return slotNum;
}


/*
 *  Update the session expiration time due to activity
 */
static void sessionActivity(Ejs *ejs, EjsWebSession *sp)
{
    sp->expire = mprGetTime(ejs) + sp->timeout * MPR_TICKS_PER_SEC;
}


/*
 *  Check for expired sessions
 */
static void sessionTimer(EjsWebControl *control, MprEvent *event)
{
    Ejs             *master;
    EjsObject       *sessions;
    EjsWebSession   *session;
    MprTime         now;
    int             i, count, deleted;

    sessions = control->sessions;
    master = control->master;
    if (master == 0) {
        mprAssert(master);
        return;
    }

    now = mprGetTime(control);

    //  TODO - locking MULTITHREAD
 
    count = ejsGetPropertyCount(master, (EjsVar*) sessions);
    deleted = 0;
    for (i = 0; i < count; i++) {
        session = (EjsWebSession*) ejsGetProperty(master, (EjsVar*) sessions, i);
        if (session && session->expire <= now) {
            ejsDeleteProperty(master, (EjsVar*) sessions, i);
            deleted++;
            i--;
            count--;
        }
    }
    if (deleted) {
        ejsCollectGarbage(master, EJS_GC_ALL);
    }
}


void ejsParseWebSessionCookie(EjsWeb *web)
{
    EjsName         qname;
    EjsWebControl   *control;
    char            *id, *cp, *value;
    int             quoted, len;

    if ((value = strstr(web->cookie, EJS_SESSION)) == 0) {
        return;
    }
    value += strlen(EJS_SESSION);

    while (isspace(*value) || *value == '=') {
        value++;
    }
    quoted = 0;
    if (*value == '"') {
        value++;
        quoted++;
    }
    for (cp = value; *cp; cp++) {
        if (quoted) {
            if (*cp == '"' && cp[-1] != '\\') {
                break;
            }
        } else {
            if ((*cp == ',' || *cp == ';') && cp[-1] != '\\') {
                break;
            }
        }
    }
    control = web->control;

    len = cp - value;
    id = mprMemdup(web, value, len + 1);
    id[len] = '\0';

    if (control->master) {
        ejsName(&qname, "", id);
        web->session = (EjsWebSession*) ejsGetPropertyByName(control->master, (EjsVar*) control->sessions, &qname);
    }
    mprFree(id);
}


/*
 *  Create a new session object. This is created in the master interpreter and will persist past the life of the current
 *  request. This will allocate a new session ID. Timeout is in seconds.
 */
EjsWebSession *ejsCreateSession(Ejs *ejs, int timeout, bool secure)
{
    Ejs             *master;
    EjsWeb          *web;
    EjsWebControl   *control;
    EjsWebSession   *session;
    EjsType         *sessionType;
    EjsName         qname;
    char            idBuf[64], *id;

    master = ejs->master;
    if (master == 0) {
        return 0;
    }
    web = ejsGetHandle(ejs);
    control = web->control;

    if (timeout <= 0) {
        timeout = control->sessionTimeout;
    }

#if ES_ejs_web_Session
    sessionType = ejsGetType(ejs, ES_ejs_web_Session);
#else
    sessionType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Session"));
#endif
    if (sessionType == 0) {
        mprAssert(0);
        return 0;
    }

    session = (EjsWebSession*) ejsCreateObject(master, sessionType, 0);
    if (session == 0) {
        return 0;
    }

    session->timeout = timeout;
    session->expire = mprGetTime(ejs) + timeout * MPR_TICKS_PER_SEC;

    mprSprintf(idBuf, sizeof(idBuf), "%08x%08x%08x", PTOI(ejs) + PTOI(web) + PTOI(session->expire), 
        (int) time(0), (int) control->nextSession++);
    id = mprGetMD5Hash(session, (uchar*) idBuf, sizeof(idBuf), "x");
    if (id == 0) {
        mprFree(session);
        return 0;
    }
    session->id = mprStrdup(session, id);

    /*
     *  Create a cookie that will only live while the browser is not exited. (Set timeout to zero).
     */
    ejsSetCookie(ejs, EJS_SESSION, id, 0, "/", secure);

    /*
     *  We use an MD5 prefix of "x" above so we can avoid the hash being interpreted as a numeric index.
     */
    ejsName(&qname, "", session->id);
    ejsSetPropertyByName(control->master, (EjsVar*) control->sessions, &qname, (EjsVar*) session);
    web->session = session;

    mprLog(ejs, 3, "Created new session %s", id);

    if (control->sessions->numProp == 1 /* TODO && !mprGetDebugMode(master) */) {
        mprCreateTimerEvent(master, (MprEventProc) sessionTimer, EJS_TIMER_PERIOD, MPR_NORMAL_PRIORITY, control, 
            MPR_EVENT_CONTINUOUS);
    }

    return session;
}


bool ejsDestroySession(Ejs *ejs)
{
    EjsWeb          *web;
    EjsWebControl   *control;
    EjsName         qname;
    int             rc;

    web = ejs->handle;
    control = web->control;

    if (web->session == 0) {
        //  TODO - remove
        mprAssert(0);
        return 0;
    }

    rc = ejsDeletePropertyByName(control->master, (EjsVar*) control->sessions, ejsName(&qname, "", web->session->id));
    web->session = 0;
    return rc;
}


EjsWebSession *ejsCreateWebSessionObject(Ejs *ejs, void *handle)
{
    EjsWebSession   *vp;
    EjsType         *requestType;
    EjsName         qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Session"));
    vp = (EjsWebSession*) ejsCreateVar(ejs, requestType, 0);
    ejsSetDebugName(vp, "EjsWeb Session Instance");

    return vp;
}


void ejsConfigureWebSessionType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Session"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Session class");
            ejs->hasError = 1;
        }
        return;
    }
    type->instanceSize = sizeof(EjsWebSession);
    mprAssert(type->hasObject);

    /*
     *  Re-define the helper functions.
     */
    type->helpers->getProperty = (EjsGetPropertyHelper) getSessionProperty;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getSessionPropertyByName;
    type->helpers->setProperty = (EjsSetPropertyHelper) setSessionProperty;
}

#endif /* BLD_FEATURE_EJS_WEB */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../web/ejsWebSession.c"
 */
/************************************************************************/

