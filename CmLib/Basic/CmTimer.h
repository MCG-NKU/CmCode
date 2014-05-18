#pragma once

#pragma warning(disable:4512)
class CmTimer
{
public:	
	CmTimer::CmTimer(CStr t = "Timer"):title(t) { is_started = false; start_clock = 0; cumulative_clock = 0; n_starts = 0; }

	~CmTimer(){	if (is_started) printf("CmTimer '%s' is started and is being destroyed.\n", title.c_str());	}

	inline void Start();
	inline void Stop();
	inline void Reset();

	inline bool Report();
	inline bool StopAndReport() { Stop(); return Report(); }
	inline float TimeInSeconds();

	inline float AvgTime(){assert(is_started == false); return TimeInSeconds()/n_starts;}

private:
	CStr title;
	
	bool is_started;
	clock_t start_clock;
	clock_t cumulative_clock;
	unsigned int n_starts;
};

/************************************************************************/
/*                       Implementations                                */
/************************************************************************/

void CmTimer::Start()
{
	if (is_started){
		printf("CmTimer '%s' is already started. Nothing done.\n", title.c_str());
		start_clock = clock();
		return;
	}

	is_started = true;
	n_starts++;
	start_clock = clock();
}

void CmTimer::Stop()
{
	if (!is_started){
		printf("CmTimer '%s' is started. Nothing done\n", title.c_str());
		return;
	}

	cumulative_clock += clock() - start_clock;
	is_started = false;
}

void CmTimer::Reset()
{
	if (is_started)	{
		printf("CmTimer '%s'is started during reset request.\n Only reset cumulative time.\n");
		return;
	}
	cumulative_clock = 0;
}

bool CmTimer::Report()
{
	if (is_started){
		printf("CmTimer '%s' is started.\n Cannot provide a time report.", title.c_str());
		return false;
	}

	float timeUsed = TimeInSeconds();
	printf("[%s] CumuTime: %4gs, #run: %2d, AvgTime: %4gs\n", title.c_str(), timeUsed, n_starts, timeUsed/n_starts);
	return true;
}

float CmTimer::TimeInSeconds()
{
	if (is_started){
		printf("CmTimer '%s' is started. Nothing done\n", title.c_str());
		return 0;
	}
	return float(cumulative_clock) / CLOCKS_PER_SEC;
}

