#include "DMStatementCompiler.h"
#include "DMCompiler.h"
#include "DMProc.h"
#include "DMObjectTree.h"
#include "DreamProcOpcode.h"
#include "DMASTExpression.h"
#include <iostream>
#include <sstream>
#include <optional>
#include <algorithm>
#include <cctype>

namespace DMCompiler {

namespace {

bool EvaluateSetConstant(DMASTExpression* expr, std::string& outString, std::optional<bool>& outBool) {
    if (!expr) {
        return false;
    }
    
    if (auto* constStr = dynamic_cast<DMASTConstantString*>(expr)) {
        outString = constStr->Value;
        outBool.reset();
        return true;
    }
    
    if (auto* constInt = dynamic_cast<DMASTConstantInteger*>(expr)) {
        outString = std::to_string(constInt->Value);
        outBool = (constInt->Value != 0);
        return true;
    }
    
    if (auto* constFloat = dynamic_cast<DMASTConstantFloat*>(expr)) {
        outString = std::to_string(constFloat->Value);
        outBool = (constFloat->Value != 0.0f);
        return true;
    }
    
    if (auto* constNull = dynamic_cast<DMASTConstantNull*>(expr)) {
        outString = "null";
        outBool = false;
        return true;
    }
    
    if (auto* constPath = dynamic_cast<DMASTConstantPath*>(expr)) {
        outString = constPath->Path.Path.ToString();
        outBool.reset();
        return true;
    }
    
    if (auto* constResource = dynamic_cast<DMASTConstantResource*>(expr)) {
        outString = constResource->Path;
        outBool.reset();
        return true;
    }
    
    if (auto* ident = dynamic_cast<DMASTIdentifier*>(expr)) {
        outString = ident->Identifier;
        std::string lower = outString;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (lower == "true") {
            outBool = true;
        } else if (lower == "false") {
            outBool = false;
        } else {
            outBool.reset();
        }
        return true;
    }
    
    return false;
}

VerbSrc AnalyzeVerbSrc(DMASTExpression* expr) {
    if (!expr) return VerbSrc::Mob; // Default

    // Handle identifiers: usr, world, contents
    if (auto* ident = dynamic_cast<DMASTIdentifier*>(expr)) {
        std::string name = ident->Identifier;
        if (name == "usr") return VerbSrc::Mob;
        if (name == "world") return VerbSrc::World;
        if (name == "contents") return VerbSrc::ObjContents;
        if (name == "group") return VerbSrc::MobGroup;
    }
    
    // Handle calls: view(), oview()
    if (auto* call = dynamic_cast<DMASTCall*>(expr)) {
        if (auto* ident = dynamic_cast<DMASTIdentifier*>(call->Target.get())) {
            std::string name = ident->Identifier;
            if (name == "view") return VerbSrc::View;
            if (name == "oview") return VerbSrc::OView;
        }
    }
    
    return VerbSrc::Mob; // Fallback
}

} // namespace

DMStatementCompiler::DMStatementCompiler(DMCompiler* compiler, DMProc* proc, 
                                         BytecodeWriter* writer, DMExpressionCompiler* exprCompiler)
    : Compiler_(compiler), Proc_(proc), Writer_(writer), ExprCompiler_(exprCompiler) {
}

// ===== Public Interface =====

bool DMStatementCompiler::CompileStatement(DMASTProcStatement* statement) {
    if (!statement) {
        return true; // Empty statement is OK
    }
    
    // Dispatch to appropriate compilation method based on statement type
    if (auto* expr = dynamic_cast<DMASTProcStatementExpression*>(statement)) {
        return CompileExpression(expr);
    } else if (auto* ret = dynamic_cast<DMASTProcStatementReturn*>(statement)) {
        return CompileReturn(ret);
    } else if (auto* ifStmt = dynamic_cast<DMASTProcStatementIf*>(statement)) {
        return CompileIf(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<DMASTProcStatementWhile*>(statement)) {
        return CompileWhile(whileStmt);
    } else if (auto* doWhileStmt = dynamic_cast<DMASTProcStatementDoWhile*>(statement)) {
        return CompileDoWhile(doWhileStmt);
    } else if (auto* forStmt = dynamic_cast<DMASTProcStatementFor*>(statement)) {
        return CompileFor(forStmt);
    } else if (auto* forRangeStmt = dynamic_cast<DMASTProcStatementForRange*>(statement)) {
        return CompileForRange(forRangeStmt);
    } else if (auto* forInStmt = dynamic_cast<DMASTProcStatementForIn*>(statement)) {
        return CompileForIn(forInStmt);
    } else if (auto* switchStmt = dynamic_cast<DMASTProcStatementSwitch*>(statement)) {
        return CompileSwitch(switchStmt);
    } else if (auto* breakStmt = dynamic_cast<DMASTProcStatementBreak*>(statement)) {
        return CompileBreak(breakStmt);
    } else if (auto* continueStmt = dynamic_cast<DMASTProcStatementContinue*>(statement)) {
        return CompileContinue(continueStmt);
    } else if (auto* varDecl = dynamic_cast<DMASTProcStatementVarDeclaration*>(statement)) {
        return CompileVarDeclaration(varDecl);
    } else if (auto* delStmt = dynamic_cast<DMASTProcStatementDel*>(statement)) {
        return CompileDel(delStmt);
    } else if (auto* spawnStmt = dynamic_cast<DMASTProcStatementSpawn*>(statement)) {
        return CompileSpawn(spawnStmt);
    } else if (auto* labelStmt = dynamic_cast<DMASTProcStatementLabel*>(statement)) {
        return CompileLabel(labelStmt);
    } else if (auto* gotoStmt = dynamic_cast<DMASTProcStatementGoto*>(statement)) {
        return CompileGoto(gotoStmt);
    } else if (auto* setStmt = dynamic_cast<DMASTProcStatementSet*>(statement)) {
        return CompileSet(setStmt);
    }
    
    std::string contextMsg = "Unknown statement type at " + statement->Location_.ToString();
    if (Proc_ && Proc_->OwningObject) {
        contextMsg += " in proc " + Proc_->OwningObject->Path.ToString() + "/" + Proc_->Name;
    }
    Compiler_->ForcedWarning(contextMsg);
    return false;
}

bool DMStatementCompiler::CompileBlockInner(DMASTProcBlockInner* block) {
    if (!block) {
        return true; // Empty block is OK
    }
    
    // Compile each statement in the block
    for (const auto& statement : block->Statements) {
        if (!CompileStatement(statement.get())) {
            return false;
        }
    }
    
    return true;
}

// ===== Label Management =====

std::string DMStatementCompiler::NewLabel() {
    // Use BytecodeWriter's label system
    int labelId = Writer_->CreateLabel();
    std::ostringstream oss;
    oss << "label_" << labelId;
    return oss.str();
}

void DMStatementCompiler::EmitLabel(const std::string& label) {
    // Extract label ID from the label string
    int labelId = std::stoi(label.substr(6)); // Skip "label_"
    Writer_->MarkLabel(labelId);
}

// ===== Jump Emission =====

void DMStatementCompiler::EmitJump(const std::string& label) {
    int labelId = std::stoi(label.substr(6)); // Skip "label_"
    Writer_->EmitJump(DreamProcOpcode::Jump, labelId);
}

void DMStatementCompiler::EmitJumpIfFalse(const std::string& label) {
    int labelId = std::stoi(label.substr(6)); // Skip "label_"
    Writer_->EmitJump(DreamProcOpcode::JumpIfFalse, labelId);
}

void DMStatementCompiler::EmitJumpIfTrue(const std::string& label) {
    // There's no JumpIfTrue opcode, so we use JumpIfFalse with inverted logic
    // Stack has a value on it already
    // We need to: if (value) jump to label
    // But we only have: if (!value) jump to target
    
    // Create a temporary label for the "don't jump" case
    std::string skipLabel = NewLabel();
    
    // If false, skip the jump
    EmitJumpIfFalse(skipLabel);
    // If we get here, value was true, so jump to destination
    EmitJump(label);
    // Skip label (if value was false, continue here)
    EmitLabel(skipLabel);
}

// ===== Loop Context Management =====

void DMStatementCompiler::PushLoopContext(const std::string& startLabel, 
                                         const std::string& endLabel,
                                         const std::string& continueLabel,
                                         bool isSwitch) {
    LoopStack_.push_back({startLabel, endLabel, continueLabel, isSwitch});
}

void DMStatementCompiler::PopLoopContext() {
    if (!LoopStack_.empty()) {
        LoopStack_.pop_back();
    }
}

DMStatementCompiler::LoopContext* DMStatementCompiler::GetCurrentLoop() {
    if (LoopStack_.empty()) {
        return nullptr;
    }
    return &LoopStack_.back();
}

// ===== Statement Compilation Methods =====

bool DMStatementCompiler::CompileExpression(DMASTProcStatementExpression* stmt) {
    // Compile the expression (pushes result onto stack)
    if (!ExprCompiler_->CompileExpression(stmt->Expression.get())) {
        return false;
    }
    
    // Pop the result since expression statements don't use their value
    Writer_->Emit(DreamProcOpcode::Pop);
    Writer_->ResizeStack(-1);  // Pops 1 value from stack
    
    return true;
}

bool DMStatementCompiler::CompileReturn(DMASTProcStatementReturn* stmt) {
    if (stmt->Value) {
        // Push return value onto stack
        if (!ExprCompiler_->CompileExpression(stmt->Value.get())) {
            return false;
        }
    } else {
        // Return null by default
        Writer_->Emit(DreamProcOpcode::PushNull);
        Writer_->ResizeStack(1);  // Pushes 1 value
    }
    
    // Emit return instruction (pops return value, but we don't track after return)
    Writer_->Emit(DreamProcOpcode::Return);
    
    return true;
}

bool DMStatementCompiler::CompileIf(DMASTProcStatementIf* stmt) {
    // Compile condition (pushes boolean result onto stack)
    if (!ExprCompiler_->CompileExpression(stmt->Condition.get())) {
        return false;
    }
    
    if (!stmt->ElseBody) {
        // Simple if without else:
        // if (condition) { body }
        //
        // Bytecode:
        //   <condition>
        //   JumpIfFalse end_label
        //   <body>
        // end_label:
        
        std::string endLabel = NewLabel();
        
        EmitJumpIfFalse(endLabel);
        Writer_->ResizeStack(-1);  // JumpIfFalse pops the condition value
        
        // Compile body
        if (!CompileBlockInner(stmt->Body.get())) {
            return false;
        }
        
        EmitLabel(endLabel);
    } else {
        // If-else:
        // if (condition) { body } else { else_body }
        //
        // Bytecode:
        //   <condition>
        //   JumpIfFalse else_label
        //   <body>
        //   Jump end_label
        // else_label:
        //   <else_body>
        // end_label:
        
        std::string elseLabel = NewLabel();
        std::string endLabel = NewLabel();
        
        EmitJumpIfFalse(elseLabel);
        Writer_->ResizeStack(-1);  // JumpIfFalse pops the condition value
        
        // Compile if body
        if (!CompileBlockInner(stmt->Body.get())) {
            return false;
        }
        
        EmitJump(endLabel);
        
        EmitLabel(elseLabel);
        
        // Compile else body
        if (!CompileBlockInner(stmt->ElseBody.get())) {
            return false;
        }
        
        EmitLabel(endLabel);
    }
    
    return true;
}

bool DMStatementCompiler::CompileWhile(DMASTProcStatementWhile* stmt) {
    // while (condition) { body }
    //
    // Bytecode:
    // start_label:
    //   <condition>
    //   JumpIfFalse end_label
    //   <body>
    //   Jump start_label
    // end_label:
    
    std::string startLabel = NewLabel();
    std::string endLabel = NewLabel();
    
    // Push loop context for break/continue
    PushLoopContext(startLabel, endLabel, startLabel);
    
    EmitLabel(startLabel);
    
    // Compile condition
    if (!ExprCompiler_->CompileExpression(stmt->Condition.get())) {
        PopLoopContext();
        return false;
    }
    
    EmitJumpIfFalse(endLabel);
    Writer_->ResizeStack(-1);  // JumpIfFalse pops the condition value
    
    // Compile body
    if (!CompileBlockInner(stmt->Body.get())) {
        PopLoopContext();
        return false;
    }
    
    EmitJump(startLabel);
    
    EmitLabel(endLabel);
    
    PopLoopContext();
    
    return true;
}

bool DMStatementCompiler::CompileDoWhile(DMASTProcStatementDoWhile* stmt) {
    // do { body } while (condition)
    //
    // Bytecode:
    // start_label:
    //   <body>
    // continue_label:
    //   <condition>
    //   JumpIfTrue start_label
    // end_label:
    
    std::string startLabel = NewLabel();
    std::string continueLabel = NewLabel();
    std::string endLabel = NewLabel();
    
    // Push loop context for break/continue
    PushLoopContext(startLabel, endLabel, continueLabel);
    
    EmitLabel(startLabel);
    
    // Compile body
    if (!CompileBlockInner(stmt->Body.get())) {
        PopLoopContext();
        return false;
    }
    
    EmitLabel(continueLabel);
    
    // Compile condition
    if (!ExprCompiler_->CompileExpression(stmt->Condition.get())) {
        PopLoopContext();
        return false;
    }
    
    EmitJumpIfTrue(startLabel);
    // Note: EmitJumpIfTrue internally pops the condition value
    Writer_->ResizeStack(-1);  // Account for the pop in EmitJumpIfTrue
    
    EmitLabel(endLabel);
    
    PopLoopContext();
    
    return true;
}

bool DMStatementCompiler::CompileFor(DMASTProcStatementFor* stmt) {
    // for (init; condition; increment) { body }
    //
    // Bytecode:
    //   <initializer>
    // start_label:
    //   <condition>
    //   JumpIfFalse end_label
    //   <body>
    // continue_label:
    //   <increment>
    //   Jump start_label
    // end_label:
    
    // Compile initializer
    if (stmt->Initializer) {
        if (!CompileStatement(stmt->Initializer.get())) {
            return false;
        }
    }
    
    std::string startLabel = NewLabel();
    std::string continueLabel = NewLabel();
    std::string endLabel = NewLabel();
    
    // Push loop context for break/continue
    PushLoopContext(startLabel, endLabel, continueLabel);
    
    EmitLabel(startLabel);
    
    // Compile condition (if present)
    if (stmt->Condition) {
        if (!ExprCompiler_->CompileExpression(stmt->Condition.get())) {
            PopLoopContext();
            return false;
        }
        EmitJumpIfFalse(endLabel);
        Writer_->ResizeStack(-1);  // JumpIfFalse pops the condition value
    }
    
    // Compile body
    if (!CompileBlockInner(stmt->Body.get())) {
        PopLoopContext();
        return false;
    }
    
    EmitLabel(continueLabel);
    
    // Compile increment (if present)
    if (stmt->Increment) {
        if (!ExprCompiler_->CompileExpression(stmt->Increment.get())) {
            PopLoopContext();
            return false;
        }
        // Pop the increment result
        Writer_->Emit(DreamProcOpcode::Pop);
        Writer_->ResizeStack(-1);  // Pops increment result
    }
    
    EmitJump(startLabel);
    
    EmitLabel(endLabel);
    
    PopLoopContext();
    
    return true;
}

bool DMStatementCompiler::CompileForRange(DMASTProcStatementForRange* stmt) {
    // for (var/x = start to end step step) { body }
    // Equivalent to:
    //   var/x = start
    // start_label:
    //   if (x > end) goto end_label  (or x < end if step is negative, but we use simple <=)
    //   <body>
    // continue_label:
    //   x += step (or x++ if step is null)
    //   goto start_label
    // end_label:
    
    // The initializer should be an assignment like var/x = start or just x = start
    // We need to extract the variable to generate comparison and increment
    
    // Compile the initializer expression (which should be a var declaration with assignment)
    if (stmt->Initializer) {
        if (!ExprCompiler_->CompileExpression(stmt->Initializer.get())) {
            return false;
        }
        // Pop the result of the initializer (we only care about side effects)
        Writer_->Emit(DreamProcOpcode::Pop);
        Writer_->ResizeStack(-1);
    }
    
    std::string startLabel = NewLabel();
    std::string continueLabel = NewLabel();
    std::string endLabel = NewLabel();
    
    // Push loop context for break/continue
    PushLoopContext(startLabel, endLabel, continueLabel);
    
    EmitLabel(startLabel);
    
    // Extract variable expression from initializer to generate the comparison
    // The initializer should be DMASTAssign
    DMASTExpression* varExpr = nullptr;
    
    if (auto* assign = dynamic_cast<DMASTAssign*>(stmt->Initializer.get())) {
        varExpr = assign->LValue.get();
    }
    
    if (varExpr) {
        // Generate: variable <= end
        if (!ExprCompiler_->CompileExpression(varExpr)) {
            PopLoopContext();
            return false;
        }
        if (!ExprCompiler_->CompileExpression(stmt->End.get())) {
            PopLoopContext();
            return false;
        }
        Writer_->Emit(DreamProcOpcode::CompareLessThanOrEqual);
        Writer_->ResizeStack(-1);  // Pops two, pushes one
        
        EmitJumpIfFalse(endLabel);
        Writer_->ResizeStack(-1);  // JumpIfFalse pops the condition
    }
    
    // Compile body
    if (!CompileBlockInner(stmt->Body.get())) {
        PopLoopContext();
        return false;
    }
    
    EmitLabel(continueLabel);
    
    // Compile increment: var += step (or var += 1 if no step)
    if (varExpr) {
        // Resolve the variable as an LValue
        auto lvalueInfo = ExprCompiler_->ResolveLValue(varExpr);
        if (lvalueInfo.Type == DMExpressionCompiler::LValueInfo::Kind::Invalid) {
            Compiler_->ForcedWarning("Invalid loop variable in for-range at " + stmt->Location_.ToString());
            PopLoopContext();
            return false;
        }
        
        // Compile step (or 1 if null)
        if (stmt->Step) {
            if (!ExprCompiler_->CompileExpression(stmt->Step.get())) {
                PopLoopContext();
                return false;
            }
        } else {
            // Default step is 1
            Writer_->EmitFloat(DreamProcOpcode::PushFloat, 1.0f);
            Writer_->ResizeStack(1);
        }
        
        // Use Append opcode (+=) with the variable reference
        Writer_->EmitMulti(DreamProcOpcode::Append, lvalueInfo.ReferenceBytes);
        
        // Pop the result of the append
        Writer_->Emit(DreamProcOpcode::Pop);
        Writer_->ResizeStack(-1);
    }
    
    EmitJump(startLabel);
    
    EmitLabel(endLabel);
    
    PopLoopContext();
    
    return true;
}

bool DMStatementCompiler::CompileBreak(DMASTProcStatementBreak* stmt) {
    LoopContext* loop = GetCurrentLoop();
    if (!loop) {
        std::string contextMsg = "break statement outside of loop at " + stmt->Location_.ToString();
        if (Proc_ && Proc_->OwningObject) {
            contextMsg += " in proc " + Proc_->OwningObject->Path.ToString() + "/" + Proc_->Name;
        }
        Compiler_->ForcedWarning(contextMsg);
        return false;
    }
    
    // Jump to the end of the loop
    EmitJump(loop->EndLabel);
    
    return true;
}

bool DMStatementCompiler::CompileContinue(DMASTProcStatementContinue* stmt) {
    // Find the nearest loop that is NOT a switch
    LoopContext* loop = nullptr;
    for (auto it = LoopStack_.rbegin(); it != LoopStack_.rend(); ++it) {
        if (!it->IsSwitch) {
            loop = &(*it);
            break;
        }
    }
    
    if (!loop) {
        std::string contextMsg = "continue statement outside of loop at " + stmt->Location_.ToString();
        if (Proc_ && Proc_->OwningObject) {
            contextMsg += " in proc " + Proc_->OwningObject->Path.ToString() + "/" + Proc_->Name;
        }
        // Compiler_->ForcedWarning(contextMsg);
        return true;
    }
    
    // Jump to the continue point (usually loop start or increment section)
    EmitJump(loop->ContinueLabel);
    
    return true;
}

// ===== Stub Implementations (to be implemented later) =====

bool DMStatementCompiler::CompileForIn(DMASTProcStatementForIn* stmt) {
    // Pattern from C# ProcessStatementForList:
    // 1. Push list onto stack
    // 2. Create list enumerator
    // 3. Start loop with LoopStart/Continue labels
    // 4. Enumerate into variable
    // 5. Compile body
    // 6. Jump to start
    // 7. End loop and destroy enumerator
    
    int enumeratorId = Proc_->GetNextEnumeratorId();
    
    // Check for range expression (start to end)
    auto* binOp = dynamic_cast<DMASTExpressionBinary*>(stmt->List.get());
    if (binOp && binOp->Operator == BinaryOperator::To) {
        // Range loop: for(x in 1 to 10)
        if (!ExprCompiler_->CompileExpression(binOp->Left.get())) return false;
        if (!ExprCompiler_->CompileExpression(binOp->Right.get())) return false;
        
        // Step is 1 by default. 
        Writer_->EmitFloat(DreamProcOpcode::PushFloat, 1.0f);
        Writer_->ResizeStack(1);
        
        // Create range enumerator (pops start, end, step)
        Writer_->EmitInt(DreamProcOpcode::CreateRangeEnumerator, enumeratorId);
        Writer_->ResizeStack(-3);
    } else {
        // Step 1: Compile the list expression (pushes list onto stack)
        if (!ExprCompiler_->CompileExpression(stmt->List.get())) {
            return false;
        }
        
        // Step 2: Create list enumerator (possibly filtered by type)
        // Check if we have a type filter from the variable declaration
        if (stmt->VarDecl.TypePath.has_value()) {
            // Use filtered enumerator with type path
            DreamPath filterPath = stmt->VarDecl.TypePath.value();
            std::string filterPathStr = filterPath.ToString();
            
            // Look up the type ID for the filter path
            DMObjectTree* objectTree = Compiler_->GetObjectTree();
            int filterTypeId = -1;
            if (objectTree) {
                int typeId;
                if (objectTree->TryGetTypeId(filterPath, typeId)) {
                    filterTypeId = typeId;
                }
            }
            
            // If type exists, use filtered enumerator; otherwise fall back to regular enumerator
            if (filterTypeId >= 0) {
                Writer_->CreateFilteredListEnumerator(enumeratorId, filterTypeId, filterPathStr);
            } else {
                // Type not found, use regular enumerator (let runtime handle filtering)
                Writer_->CreateListEnumerator(enumeratorId);
            }
        } else {
            // No type filter - use regular list enumerator
            Writer_->CreateListEnumerator(enumeratorId);
        }
        Writer_->ResizeStack(-1);  // CreateListEnumerator/CreateFilteredListEnumerator pops the list
    }
    
    // Step 3: Create labels for the loop
    std::string loopLabel = NewLabel();
    std::string continueLabel = NewLabel();
    std::string endLabel = NewLabel();
    
    // Extract label IDs for passing to Enumerate
    int endLabelId = std::stoi(endLabel.substr(6)); // Skip "label_"
    
    // Mark the start of the loop
    EmitLabel(loopLabel);
    EmitLabel(continueLabel); // Continue jumps to here
    
    // Push loop context for break/continue
    PushLoopContext(loopLabel, endLabel, continueLabel);
    
    // Step 4: Handle the loop variable using enhanced VarDecl information
    // The parser has already extracted variable name, type path, and type filter
    // into the VarDecl field during tasks 1 and 2
    
    DMReference outputRef;
    bool refCreated = false;
    bool pushedBaseObject = false;
    
    // First, check if we have enhanced variable declaration information
    if (!stmt->VarDecl.Name.empty()) {
        // Enhanced variable declaration from parser (tasks 1 & 2)
        // This handles: for(var/mob/M in world), for(var/mob/M as /mob|mob in world), etc.
        std::string varName = stmt->VarDecl.Name;
        
        // Validate variable name is not empty (requirement 1.1, 1.2)
        if (varName.empty()) {
            Compiler_->ForcedError(stmt->Location_, "Invalid variable name in for-in loop");
            PopLoopContext();
            int destroyId = Proc_->DecrementEnumeratorId();
            Writer_->DestroyEnumerator(destroyId);
            return false;
        }
        
        // Check if the variable already exists in local scope
        LocalVariable* localVar = Proc_->GetLocalVariable(varName);
        if (!localVar) {
            // Variable doesn't exist - create it with type information if available
            std::optional<DreamPath> typePath = stmt->VarDecl.TypePath;
            localVar = Proc_->AddLocalVariable(varName, typePath);
            if (!localVar) {
                Compiler_->ForcedError(stmt->Location_, "Failed to create loop variable '" + varName + "'");
                PopLoopContext();
                int destroyId = Proc_->DecrementEnumeratorId();
                Writer_->DestroyEnumerator(destroyId);
                return false;
            }
        }
        
        // Create reference to the local variable
        outputRef = DMReference::CreateLocal(localVar->Id);
        refCreated = true;
    }
    // Fallback: Handle legacy cases where VarDecl is not populated
    else if (auto* pathExpr = dynamic_cast<DMASTConstantPath*>(stmt->Variable.get())) {
        // Extract the variable name from the path (last element)
        std::string varName = pathExpr->Path.Path.GetLastElement();
        if (varName.empty()) {
            Compiler_->ForcedError(stmt->Location_, "Invalid variable path in for-in loop");
            PopLoopContext();
            int destroyId = Proc_->DecrementEnumeratorId();
            Writer_->DestroyEnumerator(destroyId);
            return false;
        }
        
        // Check if the variable already exists
        LocalVariable* localVar = Proc_->GetLocalVariable(varName);
        if (!localVar) {
            // Variable doesn't exist - create it (implicit var declaration)
            localVar = Proc_->AddLocalVariable(varName);
            if (!localVar) {
                Compiler_->ForcedError(stmt->Location_, "Failed to create loop variable '" + varName + "'");
                PopLoopContext();
                int destroyId = Proc_->DecrementEnumeratorId();
                Writer_->DestroyEnumerator(destroyId);
                return false;
            }
        }
        
        // Create reference to the local variable
        outputRef = DMReference::CreateLocal(localVar->Id);
        refCreated = true;
    }
    else if (auto* varIdent = dynamic_cast<DMASTIdentifier*>(stmt->Variable.get())) {
        // Simple identifier - check if it exists
        std::string varName = varIdent->Identifier;
        LocalVariable* localVar = Proc_->GetLocalVariable(varName);
        
        if (!localVar) {
            // Variable doesn't exist - create it implicitly
            localVar = Proc_->AddLocalVariable(varName);
            if (!localVar) {
                Compiler_->ForcedError(stmt->Location_, "Failed to create loop variable '" + varName + "'");
                PopLoopContext();
                int destroyId = Proc_->DecrementEnumeratorId();
                Writer_->DestroyEnumerator(destroyId);
                return false;
            }
        }
        
        // Create reference to the local variable
        outputRef = DMReference::CreateLocal(localVar->Id);
        refCreated = true;
    }
    else if (auto* deref = dynamic_cast<DMASTDereference*>(stmt->Variable.get())) {
        // Field access (e.g., obj.field)
        // For now, we only support simple field access on identifiers
        // More complex cases (nested derefs, array indexing) are not yet supported
        
        // Check if the base is an identifier
        if (auto* baseIdent = dynamic_cast<DMASTIdentifier*>(deref->Expression.get())) {
            // Check if the property is an identifier
            auto* propIdent = dynamic_cast<DMASTIdentifier*>(deref->Property.get());
            if (!propIdent) {
                Compiler_->ForcedError(stmt->Location_, "Field name in for-in loop must be an identifier");
                PopLoopContext();
                int destroyId = Proc_->DecrementEnumeratorId();
                Writer_->DestroyEnumerator(destroyId);
                return false;
            }
            
            std::string baseName = baseIdent->Identifier;
            std::string fieldName = propIdent->Identifier;
            
            // Check if base is a special identifier (src, usr, etc.)
            if (baseName == "src") {
                // src.field access
                int stringId = Compiler_->GetObjectTree()->AddString(fieldName);
                outputRef.RefType = DMReference::Type::SrcField;
                outputRef.Index = stringId;
                refCreated = true;
            }
            else {
                // For other cases, we'd need to compile the base expression and use Field reference
                // This is more complex and not fully implemented yet
                // Compiler_->ForcedWarning("Complex field access in for-in loop not yet fully supported");
                
                // Compile the base expression to push the object onto the stack
                if (!ExprCompiler_->CompileExpression(deref->Expression.get())) {
                    PopLoopContext();
                    int destroyId = Proc_->DecrementEnumeratorId();
                    Writer_->DestroyEnumerator(destroyId);
                    return false;
                }
                pushedBaseObject = true;

                // Try to use Field reference type (requires base object on stack)
                int stringId = Compiler_->GetObjectTree()->AddString(fieldName);
                outputRef.RefType = DMReference::Type::Field;
                outputRef.Index = stringId;
                refCreated = true;
            }
        }
        else {
            Compiler_->ForcedError(stmt->Location_, "Complex dereference in for-in loop not yet supported");
            PopLoopContext();
            int destroyId = Proc_->DecrementEnumeratorId();
            Writer_->DestroyEnumerator(destroyId);
            return false;
        }
    }
    else {
        // Unsupported expression type
        Compiler_->ForcedError(stmt->Location_, "Unsupported for-in loop variable expression type");
        PopLoopContext();
        int destroyId = Proc_->DecrementEnumeratorId();
        Writer_->DestroyEnumerator(destroyId);
        return false;
    }
    
    if (!refCreated) {
        Compiler_->ForcedError(stmt->Location_, "Failed to create reference for for-in loop variable");
        PopLoopContext();
        int destroyId = Proc_->DecrementEnumeratorId();
        Writer_->DestroyEnumerator(destroyId);
        return false;
    }
    
    // Emit Enumerate opcode (automatically jumps to endLabel when done)
    Writer_->Enumerate(enumeratorId, outputRef, endLabelId);
    
    // Step 5: Compile the loop body
    bool bodySuccess = CompileBlockInner(stmt->Body.get());
    
    // Step 6: Jump back to start of loop
    EmitJump(loopLabel);
    
    // Step 7: End of loop
    EmitLabel(endLabel);
    
    // Pop loop context (always, regardless of body compilation success)
    PopLoopContext();
    
    // If we pushed a base object for field access, pop it now
    if (pushedBaseObject) {
        Writer_->Emit(DreamProcOpcode::Pop);
        Writer_->ResizeStack(-1);
    }

    // Destroy the enumerator
    int destroyId = Proc_->DecrementEnumeratorId();
    Writer_->DestroyEnumerator(destroyId);
    
    return bodySuccess;
}

bool DMStatementCompiler::CompileSwitch(DMASTProcStatementSwitch* stmt) {
    // Compile the switch value expression (leaves value on stack)
    if (!ExprCompiler_->CompileExpression(stmt->Value.get())) {
        return false;
    }
    
    // Create end label for the switch
    std::string endLabel = NewLabel();
    
    // Push switch context for break (continue ignores switches)
    PushLoopContext("", endLabel, "", true);
    
    // Track case labels and their bodies
    struct CaseInfo {
        std::string label;
        DMASTProcBlockInner* body;
    };
    std::vector<CaseInfo> valueCases;
    DMASTProcBlockInner* defaultCaseBody = nullptr;
    
    // Process each case
    for (auto& switchCase : stmt->Cases) {
        if (switchCase.Values.empty()) {
            // Empty values = default case
            defaultCaseBody = switchCase.Body.get();
        } else {
            // Value case - create a label for it
            std::string caseLabel = NewLabel();
            
            // Emit SwitchCase opcode for each value
            for (auto& value : switchCase.Values) {
                // Check if this is a range expression
                if (auto* rangeExpr = dynamic_cast<DMASTSwitchCaseRange*>(value.get())) {
                    // Handle range: case 1 to 10
                    // Compile the lower bound
                    if (!ExprCompiler_->CompileExpression(rangeExpr->RangeStart.get())) {
                        PopLoopContext();
                        return false;
                    }
                    
                    // Compile the upper bound
                    if (!ExprCompiler_->CompileExpression(rangeExpr->RangeEnd.get())) {
                        PopLoopContext();
                        return false;
                    }
                    
                    // Emit SwitchCaseRange opcode with the case label
                    // SwitchCaseRange pops 3 values (switch value, lower bound, upper bound)
                    // and pushes switch value back if no match
                    int caseLabelId = std::stoi(caseLabel.substr(6));
                    Writer_->EmitJump(DreamProcOpcode::SwitchCaseRange, caseLabelId);
                    Writer_->ResizeStack(-2);  // Net -2 (pops lower and upper bounds, switch value stays)
                } else {
                    // Handle simple value: case 5
                    // Compile the constant value
                    if (!ExprCompiler_->CompileExpression(value.get())) {
                        PopLoopContext();
                        return false;
                    }
                    
                    // Emit SwitchCase opcode with the case label
                    // SwitchCase pops 2 values (switch value, case value), pushes switch value back if no match
                    int caseLabelId = std::stoi(caseLabel.substr(6));
                    Writer_->EmitJump(DreamProcOpcode::SwitchCase, caseLabelId);
                    Writer_->ResizeStack(-1);  // Net -1 (pops case value, switch value stays)
                }
            }
            
            valueCases.push_back({caseLabel, switchCase.Body.get()});
        }
    }
    
    // Pop the switch value (all case checks are done)
    Writer_->Emit(DreamProcOpcode::Pop);
    Writer_->ResizeStack(-1);  // Pops switch value
    
    // Compile default case first (if it exists)
    if (defaultCaseBody) {
        if (!CompileBlockInner(defaultCaseBody)) {
            PopLoopContext();
            return false;
        }
    }
    
    // Jump to end after default case
    EmitJump(endLabel);
    
    // Compile all value cases
    for (const auto& caseInfo : valueCases) {
        EmitLabel(caseInfo.label);
        
        // The switch value is still on the stack when we jump here, so we must pop it
        Writer_->Emit(DreamProcOpcode::Pop);
        Writer_->ResizeStack(-1);

        if (!CompileBlockInner(caseInfo.body)) {
            PopLoopContext();
            return false;
        }
        EmitJump(endLabel);
    }
    
    // Mark the end label
    EmitLabel(endLabel);
    
    PopLoopContext();
    
    return true;
}

bool DMStatementCompiler::CompileVarDeclaration(DMASTProcStatementVarDeclaration* stmt) {
    for (const auto& decl : stmt->Decls) {
        // Extract variable name
        std::string varName = decl.Name;
        if (varName.empty()) {
            std::string contextMsg = "Invalid variable declaration at " + stmt->Location_.ToString();
            if (Proc_ && Proc_->OwningObject) {
                contextMsg += " in proc " + Proc_->OwningObject->Path.ToString() + "/" + Proc_->Name;
            }
            Compiler_->ForcedWarning(contextMsg);
            return false;
        }
        
        // Determine the type path for this variable
        // If IsList is true (var/X[]), the type is /list even if not explicitly specified
        std::optional<DreamPath> effectiveTypePath = decl.TypePath;
        if (!effectiveTypePath && decl.IsList) {
            effectiveTypePath = DreamPath("/list");
        }
        
        // Check if the variable already exists
        LocalVariable* var = Proc_->GetLocalVariable(varName);
        if (var) {
            // Variable already exists - check if we need to update type info
            // In DM, redeclaring a variable is allowed and often used to refine the type
            if (effectiveTypePath && !var->Type) {
                var->Type = effectiveTypePath;
            }
            // Do NOT warn about redeclaration - it's valid DM behavior
        } else {
            // Add the variable to the proc's local variables
            var = Proc_->AddLocalVariable(varName, effectiveTypePath);
            if (!var) {
                std::string contextMsg = "Failed to create variable '" + varName + "' at " + stmt->Location_.ToString();
                if (Proc_ && Proc_->OwningObject) {
                    contextMsg += " in proc " + Proc_->OwningObject->Path.ToString() + "/" + Proc_->Name;
                }
                Compiler_->ForcedError(stmt->Location_, contextMsg);
                return false;
            }
        }
        
        // If there's an initialization value, compile and assign it
        if (decl.Value) {
            // Set expected type for bare 'new' inference
            ExprCompiler_->SetExpectedType(effectiveTypePath);
            
            // Compile the value expression (pushes value onto stack)
            if (!ExprCompiler_->CompileExpression(decl.Value.get())) {
                ExprCompiler_->SetExpectedType(std::nullopt);
                return false;
            }
            
            // Clear expected type after compilation
            ExprCompiler_->SetExpectedType(std::nullopt);
            
            // Emit Assign opcode followed by the reference to the local variable
            Writer_->Emit(DreamProcOpcode::Assign);
            
            // Create a reference to the local variable
            DMReference localRef = DMReference::CreateLocal(var->Id);
            
            // Write the reference to bytecode
            Writer_->WriteReference(localRef);
            
            // Assign pops value, assigns to reference, pushes assigned value (net 0)
            // But since this is a statement, we don't care about the result
        }
    }
    
    return true;
}

bool DMStatementCompiler::CompileDel(DMASTProcStatementDel* stmt) {
    // Compile the expression (the object to delete)
    if (!ExprCompiler_->CompileExpression(stmt->Value.get())) {
        return false;
    }
    
    // Emit DeleteObject opcode
    Writer_->Emit(DreamProcOpcode::DeleteObject);
    Writer_->ResizeStack(-1);  // Pops 1 value (the object to delete)
    
    return true;
}

bool DMStatementCompiler::CompileSpawn(DMASTProcStatementSpawn* stmt) {
    // Compile the delay expression (pushed onto stack)
    if (stmt->Delay) {
        if (!ExprCompiler_->CompileExpression(stmt->Delay.get())) {
            return false;
        }
    } else {
        // No delay specified - push 0
        Writer_->EmitFloat(DreamProcOpcode::PushFloat, 0.0f);
        Writer_->ResizeStack(1);  // Pushes 1 value
    }
    
    // Create label for after the spawn block
    std::string afterSpawnLabel = NewLabel();
    int afterSpawnLabelId = std::stoi(afterSpawnLabel.substr(6)); // Extract ID from "label_N"
    
    // Emit spawn opcode with jump to after-spawn label (pops delay value)
    Writer_->EmitJump(DreamProcOpcode::Spawn, afterSpawnLabelId);
    Writer_->ResizeStack(-1);  // Pops delay value
    
    // Compile the spawn body
    if (stmt->Body) {
        // Save and clear loop context to isolate spawn body from outer loops.
        // This ensures that 'continue' or 'break' statements inside the spawn
        // do not incorrectly target loops outside the spawn (Requirement 5.1).
        auto savedLoopStack = LoopStack_;
        LoopStack_.clear();

        CompileBlockInner(stmt->Body.get());

        // Restore loop context
        LoopStack_ = savedLoopStack;
    }
    
    // Prevent the spawned thread from executing outside its code
    // by adding an explicit null return
    Writer_->Emit(DreamProcOpcode::PushNull);
    Writer_->ResizeStack(1);  // Pushes null
    Writer_->Emit(DreamProcOpcode::Return);
    
    // Mark the after-spawn label (where the main thread continues)
    EmitLabel(afterSpawnLabel);
    
    return true;
}

bool DMStatementCompiler::CompileLabel(DMASTProcStatementLabel* stmt) {
    std::string generatedLabel;
    
    // Check if there are any forward references to this label
    bool hasForwardRef = false;
    for (const auto& forwardRef : ForwardReferences_) {
        if (forwardRef.LabelName == stmt->Name) {
            // Reuse the placeholder label that was created for the forward reference
            generatedLabel = forwardRef.PlaceholderLabel;
            hasForwardRef = true;
            break;
        }
    }
    
    // If no forward reference exists, create a new label
    if (!hasForwardRef) {
        generatedLabel = NewLabel();
    }
    
    // Store the mapping from the name to the generated label
    NamedLabels_[stmt->Name] = generatedLabel;
    DefinedLabels_.insert(stmt->Name);
    
    // Emit the label at this position
    EmitLabel(generatedLabel);
    
    return true;
}

bool DMStatementCompiler::CompileGoto(DMASTProcStatementGoto* stmt) {
    // Extract the label name from the identifier
    std::string labelName = stmt->Label->Identifier;
    
    // Look up the named label
    auto it = NamedLabels_.find(labelName);
    if (it == NamedLabels_.end()) {
        // Label not yet defined - create a forward reference
        // Create a temporary label that will be resolved in Finalize()
        std::string placeholderLabel = NewLabel();
        
        // Record the current bytecode position before emitting the jump
        size_t jumpPosition = Writer_->GetPosition();
        
        // Emit the jump with the placeholder label
        EmitJump(placeholderLabel);
        
        // Store the placeholder in NamedLabels_ so subsequent gotos can reuse it
        NamedLabels_[labelName] = placeholderLabel;
        
        // Track this forward reference for later resolution
        ForwardReferences_.push_back({
            labelName,
            jumpPosition,
            placeholderLabel
        });
    } else {
        // Label already exists in NamedLabels_
        // Check if this is a forward reference (not yet resolved) or a backward reference
        bool isForwardRef = false;
        for (const auto& forwardRef : ForwardReferences_) {
            if (forwardRef.LabelName == labelName) {
                isForwardRef = true;
                break;
            }
        }
        
        // Emit jump to the existing label (whether it's a placeholder or actual label)
        EmitJump(it->second);
    }
    
    return true;
}

bool DMStatementCompiler::CompileSet(DMASTProcStatementSet* stmt) {
    if (!stmt) {
        return true;
    }
    
    if (stmt->Attribute == "src") {
        VerbSrc source = AnalyzeVerbSrc(stmt->Value.get());
        if (Proc_) {
            Proc_->SetVerbSource(source);
        }
        return true;
    }

    std::string valueString;
    std::optional<bool> boolValue;
    if (!EvaluateSetConstant(stmt->Value.get(), valueString, boolValue)) {
        Compiler_->ForcedWarning("Unable to evaluate set attribute '" + stmt->Attribute + "'");
        return true;
    }
    
    if (Proc_) {
        Proc_->ApplySetAttribute(stmt->Attribute, valueString, boolValue);
    }
    
    return true;
}

bool DMStatementCompiler::Finalize() {
    // Check for any unresolved forward references
    // At this point, all labels should have been defined
    bool allResolved = true;
    
    for (const auto& forwardRef : ForwardReferences_) {
        // Look up the label to see if it was ever defined
        auto it = DefinedLabels_.find(forwardRef.LabelName);
        
        if (it == DefinedLabels_.end()) {
            // Label was never defined - this is a warning (not error) to allow compilation
            // to continue. The proc will fail at runtime if the goto is executed.
            // This commonly happens with DM labels that don't use colon syntax.
            Compiler_->ForcedWarning(
                "Undefined label '" + forwardRef.LabelName + "' referenced by goto statement"
            );
            allResolved = false;
        }
        // If the label was defined, CompileLabel() would have reused the placeholder
        // label, so the forward reference is automatically resolved
    }
    
    // Clear forward references after processing
    ForwardReferences_.clear();
    
    return allResolved;
}

} // namespace DMCompiler
