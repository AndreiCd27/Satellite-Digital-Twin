#pragma once

#include "Action.h"
#include <functional>

template<typename... ArgsT>
class Request : public AbstractFunc {
private:

	std::function<void(ArgsT...)> implementation = nullptr;
	std::tuple<ArgsT...> args;
	bool active = false;

public:
	Request() : AbstractFunc() {};

	void SetImplementation(std::function<void(ArgsT...)> _func, ArgsT... _args) {
		implementation = _func;
		args = std::make_tuple(_args...);
	}
	void Exec() override {
		if (active && implementation != nullptr) {
			// Apply stored arguments to function
			std::apply(implementation, args);
		}
		active = false;
	}
	void SetArgs(ArgsT... newARGS) {
		args = std::make_tuple(newARGS...);
	}
	void Activate() override {
		active = true;
	}
	void Deactivate() {
		active = false;
	}
	bool GetActiveStatus() {
		return active;
	}
};