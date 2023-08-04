#pragma once

// public继承表示is-a, private继承表示implemented-in-terms-of（依据...实现）。
// 因此NoCopyable类应当被子类private继承。
class NoCopyable
{
protected: // 创建NoCopyable对象是没有意义的，它只被其它类继承。
	NoCopyable() = default;
	~NoCopyable() = default;
public:
	NoCopyable(const NoCopyable&) = delete;
	NoCopyable& operator=(const NoCopyable&) = delete;
	// NoCopyable类虽然是基类，但它并非用来实现多态，不必将析构函数设置为virtual。
};
