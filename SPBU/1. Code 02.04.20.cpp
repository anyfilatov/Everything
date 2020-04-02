#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

class Point{
public:
	int x,y;
	Point(int x, int y) :x(x), y(y) {
		std::cout << "D\n";
	}
	Point(const Point& p) {
		x = p.x;
		y = p.y;
		std::cout << "C\n";
	}
	Point(Point&& p) {
		x = p.x;
		y = p.y;
		std::cout << "M\n";
	}
	int random() {
		return 4;
	}
	//decltype(this->random()) i;
};

template <typename T>
class unique {
private:
	T *ptr;
public:
	~unique() {
		delete ptr;
	}
	unique(T* t): ptr(t) {}
	unique() {
		ptr = nullptr;
	}
	unique(const unique&) = delete;
	unique& operator= (const unique&) = delete;
	unique(unique&& r){
		reset(std::move(r));
	}
	/*unique& operator= (T* t) {
		delete ptr;
		ptr = t;
	}*/
	T& operator* () {
		return *ptr;
	}
	T* operator -> () {
		return ptr;
	}
	void reset(unique&& r) {
		ptr = r.ptr;
		r.ptr = nullptr;
	}
	void swap( unique& r) {
		std::swap<T*>(ptr, r.ptr);
	}
	operator bool() {

		return ptr != nullptr;
	}
};

template <typename T, typename U>
auto divide(T a, U b) -> double{
	return a/b;
}

class MyHash {
public:
	std::size_t operator() (const Point& p) const {
		return p.x*1000 + p.y;
	}
};

class MyEqual {
public:
	bool operator() (const Point& p1, const Point& p2) const {
		return (p1.x == p2.x) && (p1.y == p2.y);
	}
};

bool operator == (const Point& p1, const Point& p2){
		return (p1.x == p2.x) && (p1.y == p2.y);
	}

bool myEq(const Point& p1, const Point& p2){
		return (p1.x == p2.x) && (p1.y == p2.y);
	}

int main() {

	/*unique<Point> p2 = new Point(1,2);
	unique<Point> p3 = new Point(10,20);
	Point * p = new Point(1,2);
	
	p2.reset(unique<Point>(p));
	p2.swap(p3);
	
	std::cout << p2->x;*/

	/*std::vector<Point> v;
	v.push_back(Point(1,2));
	v.push_back(Point(3,4));
	for (const auto& el : v) {
		decltype((el)) buf = el;
		buf.x++;
		std::cout << el.x <<" " << el.y << std::endl;
		
	}*/
	//decltype(new Point(1,2)) p = new Point(1,2);
	//std::cout << 8/2.5;
	/*decltype((divide(3,2))) i = 5.5;
	std::cout << i;*/
	

	std::unordered_map<Point, int, MyHash,myEq> voc;
	//voc.insert(std::pair<std::string,int>("aloha", 10));
	voc[Point(1,2)] = 1;
	voc[Point(3,4)] = 2;
	for (auto& el : voc) {
		std::cout << el.first.x << " " << el.second << std::endl;
	}
 	return 0; 
}
