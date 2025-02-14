#include <vcl/ast.hpp>


void VCL::ASTProgram::Accept(ASTVisitor* visitor) { visitor->VisitProgram(this); };
void VCL::ASTCompoundStatement::Accept(ASTVisitor* visitor) { visitor->VisitCompoundStatement(this); }
void VCL::ASTVariableAssignment::Accept(ASTVisitor* visitor) { visitor->VisitVariableAssignment(this); }
void VCL::ASTVariableDeclaration::Accept(ASTVisitor* visitor) { visitor->VisitVariableDeclaration(this); }
void VCL::ASTFunctionArgument::Accept(ASTVisitor* visitor) { visitor->VisitFunctionArgument(this); }
void VCL::ASTFunctionPrototype::Accept(ASTVisitor* visitor) { visitor->VisitFunctionPrototype(this); }
void VCL::ASTFunctionDeclaration::Accept(ASTVisitor* visitor) { visitor->VisitFunctionDeclaration(this); }
void VCL::ASTReturnStatement::Accept(ASTVisitor* visitor) { visitor->VisitReturnStatement(this); }
void VCL::ASTIfStatement::Accept(ASTVisitor* visitor) { visitor->VisitIfStatement(this); }
void VCL::ASTWhileStatement::Accept(ASTVisitor* visitor) { visitor->VisitWhileStatement(this); }
void VCL::ASTForStatement::Accept(ASTVisitor* visitor) { visitor->VisitForStatement(this); }
void VCL::ASTUnaryExpression::Accept(ASTVisitor* visitor) { visitor->VisitUnaryExpression(this); }
void VCL::ASTBinaryExpression::Accept(ASTVisitor* visitor) { visitor->VisitBinaryExpression(this); }
void VCL::ASTLiteralExpression::Accept(ASTVisitor* visitor) { visitor->VisitLiteralExpression(this); }
void VCL::ASTVariableExpression::Accept(ASTVisitor* visitor) { visitor->VisitVariableExpression(this); }
void VCL::ASTFunctionCall::Accept(ASTVisitor* visitor) { visitor->VisitFunctionCall(this); }