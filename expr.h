/*
  Copyright (c) 2010-2011, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
*/

/** @file expr.h
    @brief Expr abstract base class and expression implementations
*/

#ifndef ISPC_EXPR_H
#define ISPC_EXPR_H 1

#include "ispc.h"
#include "ast.h"
#include "type.h"

/** @brief Expr is the abstract base class that defines the interface that
    all expression types must implement.
 */
class Expr : public ASTNode {
public:
    Expr(SourcePos p) : ASTNode(p) { }

    /** This is the main method for Expr implementations to implement.  It
        should call methods in the FunctionEmitContext to emit LLVM IR
        instructions to the current basic block in order to generate an
        llvm::Value that represents the expression's value. */
    virtual llvm::Value *GetValue(FunctionEmitContext *ctx) const = 0;

    /** For expressions that can provide an lvalue (e.g. array indexing),
        this function should emit IR that computes the expression's lvalue
        and returns the corresponding llvm::Value.  Expressions that can't
        provide an lvalue should leave this unimplemented; the default
        implementation returns NULL.  */
    virtual llvm::Value *GetLValue(FunctionEmitContext *ctx) const;

    /** Returns the Type of the expression. */
    virtual const Type *GetType() const = 0;

    /** Returns the type of the value returned by GetLValueType(); this
        should be a pointer type of some sort (uniform or varying). */
    virtual const Type *GetLValueType() const;

    /** For expressions that have values based on a symbol (e.g. regular
        symbol references, array indexing, etc.), this returns a pointer to
        that symbol. */
    virtual Symbol *GetBaseSymbol() const;

    /** If this is a constant expression that can be converted to a
        constant of the given type, this method should return the
        corresponding llvm::Constant value.  Otherwise it should return
        NULL. */
    virtual llvm::Constant *GetConstant(const Type *type) const;

    /** This method should perform early optimizations of the expression
        (constant folding, etc.) and return a pointer to the resulting
        expression.  If an error is encountered during optimization, NULL
        should be returned. */
    virtual Expr *Optimize() = 0;

    /** This method should perform type checking of the expression and
        return a pointer to the resulting expression.  If an error is
        encountered, NULL should be returned. */
    virtual Expr *TypeCheck() = 0;

    /** Prints the expression to standard output (used for debugging). */
    virtual void Print() const = 0;
};


/** @brief Unary expression */
class UnaryExpr : public Expr {
public:
    enum Op {
        PreInc,      ///< Pre-increment
        PreDec,      ///< Pre-decrement 
        PostInc,     ///< Post-increment
        PostDec,     ///< Post-decrement
        Negate,      ///< Negation
        LogicalNot,  ///< Logical not
        BitNot,      ///< Bit not
    };

    UnaryExpr(Op op, Expr *expr, SourcePos pos);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;
    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    const Op op;
    Expr *expr;
};


/** @brief Binary expression */
class BinaryExpr : public Expr {
public:
    enum Op {
        Add,           ///< Addition
        Sub,           ///< Subtraction
        Mul,           ///< Multiplication
        Div,           ///< Division
        Mod,           ///< Modulus
        Shl,           ///< Shift left
        Shr,           ///< Shift right

        Lt,            ///< Less than
        Gt,            ///< Greater than
        Le,            ///< Less than or equal
        Ge,            ///< Greater than or equal
        Equal,         ///< Equal
        NotEqual,      ///< Not equal

        BitAnd,        ///< Bitwise AND
        BitXor,        ///< Bitwise XOR
        BitOr,         ///< Bitwise OR
        LogicalAnd,    ///< Logical AND
        LogicalOr,     ///< Logical OR

        Comma,         ///< Comma operator
    };

    BinaryExpr(Op o, Expr *a, Expr *b, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;

    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    const Op op;
    Expr *arg0, *arg1;
};


/** @brief Assignment expression */
class AssignExpr : public Expr {
public:
    enum Op {
        Assign,     ///< Regular assignment
        MulAssign,  ///< *= assignment
        DivAssign,  ///< /= assignment
        ModAssign,  ///< %= assignment
        AddAssign,  ///< += assignment
        SubAssign,  ///< -= assignment
        ShlAssign,  ///< <<= assignment
        ShrAssign,  ///< >>= assignment
        AndAssign,  ///< &= assignment
        XorAssign,  ///< ^= assignment
        OrAssign,   ///< |= assignment
    };

    AssignExpr(Op o, Expr *a, Expr *b, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;

    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    const Op op;
    Expr *lvalue, *rvalue;
};


/** @brief Selection expression, corresponding to "test ? a : b".  

    Returns the value of "a" or "b", depending on the value of "test".
*/
class SelectExpr : public Expr {
public:
    SelectExpr(Expr *test, Expr *a, Expr *b, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;

    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    Expr *test, *expr1, *expr2;
};


/** @brief A list of expressions.

    These are mostly used for representing curly-brace delimited
    initializers for initializers for complex types and for representing
    the arguments passed to a function call.
 */
class ExprList : public Expr {
public:
    ExprList(SourcePos p) : Expr(p) { }
    ExprList(Expr *e, SourcePos p) : Expr(p) { exprs.push_back(e); }

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;
    llvm::Constant *GetConstant(const Type *type) const;
    ExprList *Optimize();
    ExprList *TypeCheck();
    int EstimateCost() const;

    std::vector<Expr *> exprs;
};


/** @brief Expression representing a function call.
 */
class FunctionCallExpr : public Expr {
public:
    FunctionCallExpr(Expr *func, ExprList *args, SourcePos p, 
                     bool isLaunch = false, Expr *launchCountExpr = NULL);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;

    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    Expr *func;
    ExprList *args;
    bool isLaunch;
    Expr *launchCountExpr;
};


/** @brief Expression representing indexing into something with an integer
    offset.

    This is used for both array indexing and indexing into VectorTypes. 
*/
class IndexExpr : public Expr {
public:
    IndexExpr(Expr *baseExpr, Expr *index, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    llvm::Value *GetLValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    const Type *GetLValueType() const;
    Symbol *GetBaseSymbol() const;
    void Print() const;

    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    Expr *baseExpr, *index;
};


/** @brief Expression representing member selection ("foo.bar").
 *
 *  This will also be overloaded to deal with swizzles.
 */
class MemberExpr : public Expr {
public:
    static MemberExpr *create(Expr *expr, const char *identifier,
                              SourcePos pos, SourcePos identifierPos,
                              bool derefLvalue);
    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    llvm::Value *GetLValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    const Type *GetLValueType() const;
    Symbol *GetBaseSymbol() const;
    void Print() const;
    Expr *Optimize();
    Expr *TypeCheck();
    int EstimateCost() const;

    virtual int getElementNumber() const = 0;
    virtual const Type *getElementType() const = 0;
    std::string getCandidateNearMatches() const;

    Expr *expr;
    std::string identifier;
    const SourcePos identifierPos;

    MemberExpr(Expr *expr, const char *identifier, SourcePos pos, 
               SourcePos identifierPos, bool derefLValue);

    /** Indicates whether the expression should be dereferenced before the
        member is found.  (i.e. this is true if the MemberExpr was a '->'
        operator, and is false if it was a '.' operator. */
    bool dereferenceExpr;
};


/** @brief Expression representing a compile-time constant value.  

    This class can currently represent compile-time constants of anything
    that is an AtomicType or an EnumType; for anything more complex, we
    don't currently have a representation of a compile-time constant that
    can be further reasoned about.
 */
class ConstExpr : public Expr {
public:
    /** Create a ConstExpr from a uniform int8 value */
    ConstExpr(const Type *t, int8_t i, SourcePos p);
    /** Create a ConstExpr from a varying int8 value */
    ConstExpr(const Type *t, int8_t *i, SourcePos p);
    /** Create a ConstExpr from a uniform uint8 value */
    ConstExpr(const Type *t, uint8_t u, SourcePos p);
    /** Create a ConstExpr from a varying uint8 value */
    ConstExpr(const Type *t, uint8_t *u, SourcePos p);

    /** Create a ConstExpr from a uniform int16 value */
    ConstExpr(const Type *t, int16_t i, SourcePos p);
    /** Create a ConstExpr from a varying int16 value */
    ConstExpr(const Type *t, int16_t *i, SourcePos p);
    /** Create a ConstExpr from a uniform uint16 value */
    ConstExpr(const Type *t, uint16_t u, SourcePos p);
    /** Create a ConstExpr from a varying uint16 value */
    ConstExpr(const Type *t, uint16_t *u, SourcePos p);

    /** Create a ConstExpr from a uniform int32 value */
    ConstExpr(const Type *t, int32_t i, SourcePos p);
    /** Create a ConstExpr from a varying int32 value */
    ConstExpr(const Type *t, int32_t *i, SourcePos p);
    /** Create a ConstExpr from a uniform uint32 value */
    ConstExpr(const Type *t, uint32_t u, SourcePos p);
    /** Create a ConstExpr from a varying uint32 value */
    ConstExpr(const Type *t, uint32_t *u, SourcePos p);

    /** Create a ConstExpr from a uniform float value */
    ConstExpr(const Type *t, float f, SourcePos p);
    /** Create a ConstExpr from a varying float value */
    ConstExpr(const Type *t, float *f, SourcePos p);

    /** Create a ConstExpr from a uniform double value */
    ConstExpr(const Type *t, double d, SourcePos p);
    /** Create a ConstExpr from a varying double value */
    ConstExpr(const Type *t, double *d, SourcePos p);

    /** Create a ConstExpr from a uniform int64 value */
    ConstExpr(const Type *t, int64_t i, SourcePos p);
    /** Create a ConstExpr from a varying int64 value */
    ConstExpr(const Type *t, int64_t *i, SourcePos p);
    /** Create a ConstExpr from a uniform uint64 value */
    ConstExpr(const Type *t, uint64_t i, SourcePos p);
    /** Create a ConstExpr from a varying uint64 value */
    ConstExpr(const Type *t, uint64_t *i, SourcePos p);

    /** Create a ConstExpr from a uniform bool value */
    ConstExpr(const Type *t, bool b, SourcePos p);
    /** Create a ConstExpr from a varying bool value */
    ConstExpr(const Type *t, bool *b, SourcePos p);

    /** Create a ConstExpr of the same type as the given old ConstExpr,
        with values given by the "vales" parameter. */
    ConstExpr(ConstExpr *old, double *values);

    /** Create ConstExpr with the same type and values as the given one,
        but at the given position. */
    ConstExpr(ConstExpr *old, SourcePos pos);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;
    llvm::Constant *GetConstant(const Type *type) const;

    Expr *TypeCheck();
    Expr *Optimize();
    int EstimateCost() const;

    /** Return the ConstExpr's values as booleans, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsBool(bool *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as int8s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsInt8(int8_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as uint8s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsUInt8(uint8_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as int16s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsInt16(int16_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as uint16s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsUInt16(uint16_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as int32s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsInt32(int32_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as uint32s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsUInt32(uint32_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as floats, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsFloat(float *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as int64s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsInt64(int64_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as uint64s, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsUInt64(uint64_t *, bool forceVarying = false) const;

    /** Return the ConstExpr's values as doubles, doing type conversion
        from the actual type if needed.  If forceVarying is true, then type
        convert to 'varying' so as to always return a number of values
        equal to the target vector width into the given pointer. */
    int AsDouble(double *, bool forceVarying = false) const;

    /** Return the number of values in the ConstExpr; should be either 1,
        if it has uniform type, or the target's vector width if it's
        varying. */
    int Count() const;

private:
    AtomicType::BasicType getBasicType() const;

    const Type *type;
    union {
        int8_t int8Val[ISPC_MAX_NVEC];
        uint8_t uint8Val[ISPC_MAX_NVEC];
        int16_t int16Val[ISPC_MAX_NVEC];
        uint16_t uint16Val[ISPC_MAX_NVEC];
        int32_t int32Val[ISPC_MAX_NVEC];
        uint32_t uint32Val[ISPC_MAX_NVEC];
        bool boolVal[ISPC_MAX_NVEC];
        float floatVal[ISPC_MAX_NVEC];
        double doubleVal[ISPC_MAX_NVEC];
        int64_t int64Val[ISPC_MAX_NVEC];
        uint64_t uint64Val[ISPC_MAX_NVEC];
    };
};


/** @brief Expression representing a type cast of the given expression to a
    probably-different type. */
class TypeCastExpr : public Expr {
public:
    TypeCastExpr(const Type *t, Expr *e, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;
    Expr *TypeCheck();
    Expr *Optimize();
    int EstimateCost() const;
    Symbol *GetBaseSymbol() const;
    llvm::Constant *GetConstant(const Type *type) const;

    const Type *type;
    Expr *expr;
};


/** @brief Expression that represents taking a reference of a (non-reference)
    variable. */
class ReferenceExpr : public Expr {
public:
    ReferenceExpr(Expr *e, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    const Type *GetLValueType() const;
    Symbol *GetBaseSymbol() const;
    void Print() const;
    Expr *TypeCheck();
    Expr *Optimize();
    int EstimateCost() const;

    Expr *expr;
};


/** @brief Expression that represents dereferencing a reference to get its
    value. */
class DereferenceExpr : public Expr {
public:
    DereferenceExpr(Expr *e, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    llvm::Value *GetLValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    const Type *GetLValueType() const;
    Symbol *GetBaseSymbol() const;
    void Print() const;
    Expr *TypeCheck();
    Expr *Optimize();
    int EstimateCost() const;

    Expr *expr;
};


/** Expression that represents taking the address of an expression. */
class AddressOfExpr : public Expr {
public:
    AddressOfExpr(Expr *e, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    Symbol *GetBaseSymbol() const;
    void Print() const;
    Expr *TypeCheck();
    Expr *Optimize();
    int EstimateCost() const;

    Expr *expr;
};


/** Expression that returns the size of the given expression or type in
    bytes. */
class SizeOfExpr : public Expr {
public:
    SizeOfExpr(Expr *e, SourcePos p);
    SizeOfExpr(const Type *t, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    void Print() const;
    Expr *TypeCheck();
    Expr *Optimize();
    int EstimateCost() const;

    /* One of expr or type should be non-NULL (but not both of them).  The
       SizeOfExpr returns the size of whichever one of them isn't NULL. */
    Expr *expr;
    const Type *type;
};


/** @brief Expression representing a symbol reference in the program */
class SymbolExpr : public Expr {
public:
    SymbolExpr(Symbol *s, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    llvm::Value *GetLValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    const Type *GetLValueType() const;
    Symbol *GetBaseSymbol() const;
    Expr *TypeCheck();
    Expr *Optimize();
    void Print() const;
    int EstimateCost() const;

private:
    Symbol *symbol;
};


/** @brief Expression representing a function symbol in the program (generally
    used for a function call).
 */    
class FunctionSymbolExpr : public Expr {
public:
    FunctionSymbolExpr(const char *name, const std::vector<Symbol *> &candFuncs,
                       SourcePos pos);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    Symbol *GetBaseSymbol() const;
    Expr *TypeCheck();
    Expr *Optimize();
    void Print() const;
    int EstimateCost() const;
    llvm::Constant *GetConstant(const Type *type) const;

    /** Given the types of the function arguments, in the presence of
        function overloading, this method resolves which actual function
        the arguments match best.  If the argCouldBeNULL parameter is
        non-NULL, each element indicates whether the corresponding argument
        is the number zero, indicating that it could be a NULL pointer.
        This parameter may be NULL (for cases where overload resolution is
        being done just given type information without the parameter
        argument expressions being available.  It returns true on success.
     */
    bool ResolveOverloads(SourcePos argPos,
                          const std::vector<const Type *> &argTypes,
                          const std::vector<bool> *argCouldBeNULL = NULL);
    Symbol *GetMatchingFunction();

private:
    bool tryResolve(int (*matchFunc)(const Type *, const Type *),
                    SourcePos argPos, const std::vector<const Type *> &argTypes,
                    const std::vector<bool> *argCouldBeNULL);

    /** Name of the function that is being called. */
    std::string name;

    /** All of the functions with the name given in the function call;
        there may be more then one, in which case we need to resolve which
        overload is the best match. */
    std::vector<Symbol *> candidateFunctions;

    /** The actual matching function found after overload resolution. */
    Symbol *matchingFunc;

    bool triedToResolve;
};


/** @brief A sync statement in the program (waits for all launched tasks before
    proceeding). */
class SyncExpr : public Expr {
public:
    SyncExpr(SourcePos p) : Expr(p) { }

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    Expr *TypeCheck();
    Expr *Optimize();
    void Print() const;
    int EstimateCost() const;
};


/** @brief An expression that represents a NULL pointer. */
class NullPointerExpr : public Expr {
public:
    NullPointerExpr(SourcePos p) : Expr(p) { }
    
    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    Expr *TypeCheck();
    Expr *Optimize();
    void Print() const;
    int EstimateCost() const;
};


/** An expression representing a "new" expression, used for dynamically
    allocating memory. 
*/
class NewExpr : public Expr {
public:
    NewExpr(int typeQual, const Type *type, Expr *initializer, Expr *count, 
            SourcePos tqPos, SourcePos p);

    llvm::Value *GetValue(FunctionEmitContext *ctx) const;
    const Type *GetType() const;
    Expr *TypeCheck();
    Expr *Optimize();
    void Print() const;
    int EstimateCost() const;

    /** Type of object to allocate storage for. */
    const Type *allocType;
    /** Expression giving the number of elements to allocate, when the 
        "new Foo[expr]" form is used.  This may be NULL, in which case a
        single element of the given type will be allocated. */
    Expr *countExpr;
    /** Optional initializer expression used to initialize the allocated
        memory. */
    Expr *initExpr;
    /** Indicates whether this is a "varying new" or "uniform new"
        (i.e. whether a separate allocation is performed per program
        instance, or whether a single allocation is performed for the
        entire gang of program instances.) */
    bool isVarying;
};


/** This function indicates whether it's legal to convert from fromType to
    toType.  If the optional errorMsgBase and source position parameters
    are provided, then an error message is issued if the type conversion
    isn't possible.
 */
bool CanConvertTypes(const Type *fromType, const Type *toType,
                     const char *errorMsgBase = NULL,
                     SourcePos pos = SourcePos());

/** This function attempts to convert the given expression to the given
    type, returning a pointer to a new expression that is the result.  If
    the required type conversion is illegal, it returns NULL and prints an
    error message using the provided string to indicate the context for
    which type conversion was being applied (e.g. "function call
    parameter").
 */
Expr *TypeConvertExpr(Expr *expr, const Type *toType, const char *errorMsgBase);

/** Utility routine that emits code to initialize a symbol given an
    initializer expression.

    @param lvalue    Memory location of storage for the symbol's data
    @param symName   Name of symbol (used in error messages)
    @param symType   Type of variable being initialized
    @param initExpr  Expression for the initializer
    @param ctx       FunctionEmitContext to use for generating instructions
    @param pos       Source file position of the variable being initialized
*/
void
InitSymbol(llvm::Value *lvalue, const Type *symType, Expr *initExpr,
           FunctionEmitContext *ctx, SourcePos pos);

bool PossiblyResolveFunctionOverloads(Expr *expr, const Type *type);

#endif // ISPC_EXPR_H
