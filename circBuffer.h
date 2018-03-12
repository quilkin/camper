

/*
CircularBuffer.h - circular buffer library for Arduino.
*/

#ifndef CIRCULARBUFFER_h
#define CIRCULARBUFFER_h
#include <inttypes.h>

template <typename T, uint16_t Size>
class CircularBuffer {
public:
	//enum {
	//	Empty = 0,
	//	Half = Size / 2,
	//	Full = Size,
	//};

	CircularBuffer() :
		tail(buf_), head(buf_), count(0) {}
	~CircularBuffer() {}
	void push(T value) {
		if (++tail == buf_ + Size) {
			tail = buf_;
		}
		*tail = value;

		if (count == Size) {
			if (++head == buf_ + Size) {
				head = buf_;
			}
		} else {
			if (count++ == 0) {
				head = tail;
			}
		}
    }
  
	T* pop() {
		if (count <= 0) return 0;
		T* result = tail--;
		if (tail < buf_) {
			tail = buf_ + Size - 1;
		}
		count--;
		return result;
	}

	T* shift() {
		if (count <= 0) return 0;
		T* result = head++;
		if (head >= buf_ + Size) {
			head = buf_;
		}
		count--;
		return result;
	}

void unshift(T value) {
	if (head == buf_) {
		head = buf_ + Size;
		}
	*--head = value;
	if (count == Size) {
		if (tail-- == buf_) {
			tail = buf_ + Size - 1;
		}
	} 
	else {
		if (count++ == 0) {
		tail = head;
		}
	 }
}
 
int remain() const {
		return count;
	}
 
T peek(uint16_t a) {
		T* peekP = tail  - a;
		if (peekP < buf_)
			peekP = peekP + Size;
		T result = *peekP;
		return result;
	}

private:
	T buf_[Size];
	T *tail;
	T *head;
	uint16_t count;
};

#endif


