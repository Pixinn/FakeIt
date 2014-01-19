#ifndef Clouses_h__
#define Clouses_h__

#ifdef _WIN32
#define THROWS throw(...)
#else
#define THROWS noexcept(false)
#endif

#include <functional>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "mockutils/traits.h"
#include "mockutils/DefaultValue.hpp"
#include "mock4cpp/Exceptions.h"
#include "mock4cpp/InvocationMatcher.h"
#include "mock4cpp/ActualInvocation.h"

namespace mock4cpp {

struct MethodVerificationProgress {

	MethodVerificationProgress() {
	}

	~MethodVerificationProgress() THROWS {
	}

	void Never() {
		Times(0);
	}

	virtual void Once() {
		Times(1);
	}

	virtual void Twice() {
		Times(2);
	}

	virtual void AtLeastOnce() {
		verifyInvocations(-1);
	}

	virtual void Times(const int times) {
		if (times < 0) {
			throw IllegalArgumentException(std::string("bad argument times:").append(std::to_string(times)));
		}
		verifyInvocations(times);
	}

protected:
	virtual void startVerification() = 0;
	virtual void verifyInvocations(const int times) = 0;
private:
	MethodVerificationProgress & operator=(const MethodVerificationProgress & other) = delete;
};

template<typename R, typename ... arglist>
struct NextFunctionStubbingProgress {

	virtual ~NextFunctionStubbingProgress() THROWS {
	}

	NextFunctionStubbingProgress<R, arglist...>&
	ThenReturn(const R& r) {
		return ThenDo([&r](...)->R {return r;});
	}

	NextFunctionStubbingProgress<R, arglist...>&
	ThenReturn() {
		return Do([](...)->R {DefaultValue::value<R>();});
	}

	template<typename E>
	NextFunctionStubbingProgress<R, arglist...>& ThenThrow(const E& e) {
		return ThenDo([e](...)->R {throw e;});
	}

	NextFunctionStubbingProgress<R, arglist...>& ThenDo(R (*method)(arglist...)) {
		return ThenDo(std::function < R(arglist...) > (method));
	}

	virtual auto ThenDo(std::function<R(arglist...)> method) -> NextFunctionStubbingProgress<R, arglist...>& = 0;

private:
	NextFunctionStubbingProgress & operator=(const NextFunctionStubbingProgress & other) = delete;
};

template<typename R, typename ... arglist>
struct FirstFunctionStubbingProgress {

	virtual ~FirstFunctionStubbingProgress() THROWS {
	}

	NextFunctionStubbingProgress<R, arglist...>&
	Return(const R& r) {
		return Do([&r](...)->R {return r;});
	}

	NextFunctionStubbingProgress<R, arglist...>&
	Return() {
		return Do([](...)->R {DefaultValue::value<R>();});
	}

	template<typename E>
	NextFunctionStubbingProgress<R, arglist...>& Throw(const E& e) {
		return Do([e](...)->R {throw e;});
	}

	virtual void operator=(std::function<R(arglist...)> method) {
		Do(method);
	}

	virtual auto Do(std::function<R(arglist...)> method) -> NextFunctionStubbingProgress<R, arglist...>& = 0;

private:
	FirstFunctionStubbingProgress & operator=(const FirstFunctionStubbingProgress & other) = delete;
};

template<typename R, typename ... arglist>
struct NextProcedureStubbingProgress {
	virtual ~NextProcedureStubbingProgress() THROWS {
	}

	NextProcedureStubbingProgress<R, arglist...>& ThenReturn() {
		return ThenDo([](...)->R {return DefaultValue::value<R>();});
	}

	template<typename E>
	NextProcedureStubbingProgress<R, arglist...>& ThenThrow(const E e) {
		return ThenDo([e](...)->R {throw e;});
	}

	virtual NextProcedureStubbingProgress<R, arglist...>& ThenDo(std::function<R(arglist...)> method) = 0;
private:
	NextProcedureStubbingProgress & operator=(const NextProcedureStubbingProgress & other) = delete;
};

template<typename R, typename ... arglist>
struct FirstProcedureStubbingProgress {

	virtual ~FirstProcedureStubbingProgress() THROWS {
	}

	NextProcedureStubbingProgress<R, arglist...>& Return() {
		return Do([](...)->R {return DefaultValue::value<R>();});
	}

	template<typename E>
	NextProcedureStubbingProgress<R, arglist...>& Throw(const E e) {
		return Do([e](...)->R {throw e;});
	}

	virtual void operator=(std::function<R(arglist...)> method) {
		Do(method);
	}

	virtual NextProcedureStubbingProgress<R, arglist...>& Do(std::function<R(arglist...)> method) = 0;

private:
	FirstProcedureStubbingProgress & operator=(const FirstProcedureStubbingProgress & other) = delete;
};

class MethodStubbingInternal {
public:

	MethodStubbingInternal() = default;

protected:

	~MethodStubbingInternal() = default;

	virtual void startStubbing() = 0;

};

class Sequence {
private:
	bool _isActive;

protected:
	virtual void startVerification() {
		_isActive = true;
	}

	virtual void cancelVerification() {
		_isActive = false;
	}

	bool isActive() {
		return _isActive;
	}

	Sequence() :
			_isActive(false) {
	}

public:
	virtual void getActualInvocationSequence(std::unordered_set<AnyInvocation*>& into) const = 0;

	virtual void getExpectedInvocationSequence(std::vector<AnyInvocationMatcher*>& into) const = 0;

	friend class VerifyFunctor;
};

class ConcatenatedSequence: public virtual Sequence {
private:
	const Sequence &s1;
	const Sequence &s2;

protected:
	ConcatenatedSequence(const Sequence &s1, const Sequence &s2) :
			s1(s1), s2(s2) {
	}

public:

	~ConcatenatedSequence() {
		if (isActive()) {

		}
	}

	void getActualInvocationSequence(std::unordered_set<AnyInvocation*>& into) const override {
		s1.getActualInvocationSequence(into);
		s2.getActualInvocationSequence(into);
	}

	void getExpectedInvocationSequence(std::vector<AnyInvocationMatcher*>& into) const override {
		s1.getExpectedInvocationSequence(into);
		s2.getExpectedInvocationSequence(into);
	}

	friend inline ConcatenatedSequence operator+(const Sequence &s1, const Sequence &s2);
};

class RepeatedSequence: public virtual Sequence {
private:
	const Sequence &s1;
	const int times;

protected:
	RepeatedSequence(const Sequence &s1, const int times) :
			s1(s1), times(times) {
	}

public:

	~RepeatedSequence() {
		if (isActive()) {

		}
	}

	friend inline RepeatedSequence operator*(const Sequence &s1, int times);

	void getActualInvocationSequence(std::unordered_set<AnyInvocation*>& into) const override {
		s1.getActualInvocationSequence(into);
	}

	void getExpectedInvocationSequence(std::vector<AnyInvocationMatcher*>& into) const override {
		for (int i = 0; i < times; i++)
			s1.getExpectedInvocationSequence(into);
	}
};

inline ConcatenatedSequence operator+(const Sequence &s1, const Sequence &s2) {
	return ConcatenatedSequence(s1, s2);
}

inline RepeatedSequence operator*(const Sequence &s1, int times) {
	return RepeatedSequence(s1, times);
}

}

#endif // Clouses_h__
