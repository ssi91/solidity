/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Class that contains contextual information during IR generation.
 */

#pragma once

#include <libsolidity/codegen/ir/IRVariable.h>
#include <libsolidity/interface/OptimiserSettings.h>
#include <libsolidity/interface/DebugSettings.h>

#include <libsolidity/codegen/MultiUseYulFunctionCollector.h>

#include <liblangutil/EVMVersion.h>

#include <libsolutil/Common.h>

#include <set>
#include <string>
#include <memory>
#include <vector>

namespace solidity::frontend
{

class ContractDefinition;
class VariableDeclaration;
class FunctionDefinition;
class Expression;
class YulUtilFunctions;

/**
 * A very thin wrapper over a collection of function definitions.
 * It provides a queue-like interface but does not guarantee element order and does not
 * preserve duplicates.
 */
class IRFunctionGenerationQueue
{
public:
	void push(FunctionDefinition const* _definition) { m_definitions.insert(_definition); }
	FunctionDefinition const* pop();
	void clear() { m_definitions.clear(); }

	bool empty() const { return m_definitions.empty(); }
	size_t size() const { return m_definitions.size(); };

private:
	// Since we don't care about duplicates or order, std::set serves our needs better than std::queue.
	std::set<FunctionDefinition const*> m_definitions;
};

/**
 * Class that contains contextual information during IR generation.
 */
class IRGenerationContext
{
public:
	IRGenerationContext(
		langutil::EVMVersion _evmVersion,
		RevertStrings _revertStrings,
		OptimiserSettings _optimiserSettings
	):
		m_evmVersion(_evmVersion),
		m_revertStrings(_revertStrings),
		m_optimiserSettings(std::move(_optimiserSettings))
	{}

	MultiUseYulFunctionCollector& functionCollector() { return m_functions; }

	/// Provides access to the function definitions queued for code generation. They're the
	/// functions whose calls were discovered by the IR generator during AST traversal.
	/// Note that the queue gets filled in a lazy way - new definitions can be added while the
	/// collected ones get removed and traversed.
	IRFunctionGenerationQueue& functionGenerationQueue() { return m_functionGenerationQueue; }
	IRFunctionGenerationQueue const& functionGenerationQueue() const { return m_functionGenerationQueue; }

	/// Adds a function to the function generation queue and returns the name of the function.
	std::string enqueueFunctionForCodeGeneration(FunctionDefinition const& _function);

	/// Resolves a virtual function call into the right definition and queues it for code generation
	/// code generation. Returns the name of the queued function.
	std::string enqueueVirtualFunctionForCodeGeneration(FunctionDefinition const& _functionDeclaration);

	/// Sets the most derived contract (the one currently being compiled)>
	void setMostDerivedContract(ContractDefinition const& _mostDerivedContract)
	{
		m_mostDerivedContract = &_mostDerivedContract;
	}
	ContractDefinition const& mostDerivedContract() const;


	IRVariable const& addLocalVariable(VariableDeclaration const& _varDecl);
	bool isLocalVariable(VariableDeclaration const& _varDecl) const { return m_localVariables.count(&_varDecl); }
	IRVariable const& localVariable(VariableDeclaration const& _varDecl);

	void addStateVariable(VariableDeclaration const& _varDecl, u256 _storageOffset, unsigned _byteOffset);
	bool isStateVariable(VariableDeclaration const& _varDecl) const { return m_stateVariables.count(&_varDecl); }
	std::pair<u256, unsigned> storageLocationOfVariable(VariableDeclaration const& _varDecl) const
	{
		return m_stateVariables.at(&_varDecl);
	}

	std::string functionName(FunctionDefinition const& _function);
	std::string functionName(VariableDeclaration const& _varDecl);

	std::string newYulVariable();

	std::string internalDispatch(size_t _in, size_t _out);

	/// @returns a new copy of the utility function generator (but using the same function set).
	YulUtilFunctions utils();

	langutil::EVMVersion evmVersion() const { return m_evmVersion; };

	/// @returns code that stores @param _message for revert reason
	/// if m_revertStrings is debug.
	std::string revertReasonIfDebug(std::string const& _message = "");

	RevertStrings revertStrings() const { return m_revertStrings; }

	/// @returns the variable name that can be used to inspect the success or failure of an external
	/// function call that was invoked as part of the try statement.
	std::string trySuccessConditionVariable(Expression const& _expression) const;

private:
	langutil::EVMVersion m_evmVersion;
	RevertStrings m_revertStrings;
	OptimiserSettings m_optimiserSettings;
	ContractDefinition const* m_mostDerivedContract = nullptr;
	std::map<VariableDeclaration const*, IRVariable> m_localVariables;
	/// Storage offsets of state variables
	std::map<VariableDeclaration const*, std::pair<u256, unsigned>> m_stateVariables;
	MultiUseYulFunctionCollector m_functions;
	IRFunctionGenerationQueue m_functionGenerationQueue;
	size_t m_varCounter = 0;
};

}
