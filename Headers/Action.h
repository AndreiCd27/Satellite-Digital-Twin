#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <tuple>
#include <string>

// Abstract class that removes the need for args templates
// Templates are not explicitly said in other classes,
// And lets any service/main.cpp implementation set them
// And be evaluated at compile time
/* EXAMPLE:
 class ActionContainer {
	std::vector<std::shared_ptr<AbstractFunc>> ACTIONS; <--- No template Args specified
*/
class AbstractFunc {
public:
	virtual ~AbstractFunc() = default;
	virtual void Exec() = 0;
	virtual void Activate() = 0;
};

template <typename... ArgsT>
class Action : public AbstractFunc {
private:

	std::function<void(ArgsT...)> func;
	std::tuple<ArgsT...> args;

	std::string Name = "Annonymous_Function";

public:
	Action(std::function<void(ArgsT...)> _func, ArgsT... _args) : AbstractFunc() {
		func = _func;
		args = std::make_tuple(_args...);
	}
	Action(std::function<void(ArgsT...)> _func, ArgsT... _args, const std::string& Name) : Name(Name), AbstractFunc() {
		func = _func;
		args = std::make_tuple(_args...);
	}
	void Exec() override {
		// Apply stored arguments to function
		std::apply(func, args);
	}
	void SetArgs(ArgsT... newARGS) {
		args = std::make_tuple(newARGS...);
	}
	void Activate() override {
		// Does nothing, reserved for Request class (see Request.h)
	}
};

class ActionContainer {
	std::vector<std::shared_ptr<AbstractFunc>> ACTIONS;
public:
	ActionContainer() = default;

	template <typename... ArgsT>
	std::shared_ptr<Action<ArgsT...>> AddAction(std::function<void(ArgsT...)> func, ArgsT... args) {
		// Adds function to container
		auto action = std::make_shared<Action<ArgsT...>>(func, args...);
		ACTIONS.push_back(action);
		// Returns an index to be stored for later function identification
		return action;
	}

	void ExecAll() {
		for (auto& f : ACTIONS) {
			f->Exec();
		}
	}

	void ExecAtIndex(int index) {
		ACTIONS.at(index)->Exec();
	}
};