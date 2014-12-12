#include <functional>
#include <utility>
#include <map>
#include <string>

// Every event has a callable and arguments.
// There's an EventDispatcher which handles all events and calls them.


// There's two types of events, one which returns EventReturn and one which is a void function.
enum EventReturn
{
	EVENT_STOP = 1,
	EVENT_CONTINUE
};

typedef std::function<void()> voidfunc_t;
typedef std::function<EventReturn()> eventfunc_t;

class EventDispatcher
{
	std::map<std::string, eventfunc_t> retevents;
	std::map<std::string, voidfunc_t> voidevents;
public:
	EventDispatcher() {}
	~EventDispatcher() {}
	
	// Handle adding events to functions
	template<class _Function>
	void AddReturnEvent(const std::string &name, _Function&& __f)
	{
		eventfunc_t func = std::function<EventReturn()>(__f);
		//std::bind(std::forward<_Function>(__f), std::forward<_Args>(__args)...);
		retevents[name] = func;
	}
	
	template<class _Function>
	void AddVoidEvent(const std::string &name, _Function&& __f)
	{
		voidfunc_t func = std::function<void()>(__f);
		//std::bind(std::forward<_Function>(__f), std::forward<_Args>(__args)...);
		voidevents[name] = func;
	}
	
	template<class... _Args>
	void CallVoidEvent(const std::string &name, _Args&&... __args)
	{
		for (std::map<std::string, voidfunc_t>::const_iterator it = voidevents.begin(), it_end = voidevents.end(); it != it_end; ++it)
		{
			if (it->first == name)
			{
				voidfunc_t func = std::bind(it->second, std::forward<_Args>(__args)...);
				func();
			}
		}
	}
	
	
	template<class... _Args>
	EventReturn CallReturnEvent(const std::string &name, _Args&&... __args)
	{
		for (std::map<std::string, eventfunc_t>::const_iterator it = retevents.begin(), it_end = retevents.end(); it != it_end; ++it)
		{
			if (it->first == name)
			{
				eventfunc_t func = std::bind(it->second, std::forward<_Args>(__args)...);
				EventReturn ret = func();
				if (ret != EVENT_CONTINUE)
					return ret;
			}
		}
		
		return EVENT_CONTINUE;
	}
};

