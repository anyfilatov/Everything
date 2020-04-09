#include <vector>
#include <iterator>
#include <iostream>

template<typename T, typename Iterator>
class range_enumerator;

template<typename T>
class take_enumerator;


template<typename T>
class enumerator {
public:
	enumerator(const enumerator&) = delete;
	enumerator& operator=(const enumerator&) = delete;
	enumerator& operator=(enumerator&&) = delete;
	enumerator(enumerator&&) = default;
	enumerator() = default;
	
	virtual ~enumerator() = default;
	virtual const T& operator* () const = 0;
	virtual enumerator& operator++() = 0;
	virtual operator bool() const = 0;

	std::vector<T> to_vector() {
		std::vector<T> result;
		while(*this) {
			result.push_back(**this);
			++*this;
		}
		return result;
	}
	auto take(int number) {
		return take_enumerator<T>(*this, number);
	}
	/*template <typename F>	
	auto where( F func) {
		return where_enumerator<T,F>(*this, std::move(func));
		In where enumerator operator * and bool - are boring. But operator++ should do
		parent++ while func(parent) == false;
	}*/
};

template<typename T, typename Iterator>
class range_enumerator: public enumerator<T> {
public:
	range_enumerator(Iterator beg, Iterator end) : beg_(beg), end_(end) {}
	 const T& operator* () const {
		return *beg_;
	}
	range_enumerator& operator++() {
		++beg_;
		return *this;
	}
	operator bool() const {
		return beg_ != end_;
	}
private:
	Iterator beg_, end_;
};

template<typename Iterator>
auto from(Iterator beg, Iterator end) {
	return range_enumerator<typename std::iterator_traits<Iterator>::value_type, Iterator> (beg, end);
}

template<typename T>
class take_enumerator: public enumerator<T> {
public:
	take_enumerator(enumerator<T>& parent, int number) : parent_(parent), number_(number) {}
	const T& operator* () const override{
		return *parent_;
	}
	take_enumerator& operator++() override {
		--number_;
		if (number_ > 0) {
			++parent_;
		}
		return *this;
	}
	operator bool() const override {
		return number_ > 0 && (bool) parent_;
	}
private:
	enumerator<T>& parent_;
	int number_;
};

int main() {
	int x[] = {1,2,3,4,5};
	auto a = from(x, x+5).take(2).to_vector();
	std::cout << a[1];
}
