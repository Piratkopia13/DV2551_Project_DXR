#pragma once

#include <string>
#include <map>
#include <vector>

class DX12Renderer;

class TimerSaver {

public:
	TimerSaver(unsigned int numResultsToSave, DX12Renderer* renderer);
	~TimerSaver();

	void addResult(const std::string& name, int column, double valueMs);
	void saveToFile(const std::string& filePrefix = "");
	unsigned int getSizeOf(const std::string& name, int column);

private:
	DX12Renderer* m_renderer;
	bool m_full;
	unsigned int m_maxResults;
	std::map<std::string, std::map<int, std::vector<double>>> m_results;

};