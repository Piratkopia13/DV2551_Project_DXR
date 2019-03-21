#include "pch.h"
#include "TimerSaver.h"

#include <iomanip>
#include <time.h>
#include <ctime>

TimerSaver::TimerSaver(unsigned int numResultsToSave) 
	: m_maxResults(numResultsToSave)
	, m_full(false)
{
#ifdef _DEBUG
	std::cout << "RUNNING IN DEBUG - NO TIMERS WILL BE SAVED" << std::endl;
	m_full = true;
#endif
}

TimerSaver::~TimerSaver() {
}

void TimerSaver::addResult(const std::string& name, int column, double valueMs) {

	if (m_full) return;

	if (m_results[name].find(column) == m_results[name].end()) {
		m_results[name][column].reserve(m_maxResults);
	}
	m_results[name][column].emplace_back(valueMs);

	if (m_results[name][column].size() >= m_maxResults) {
		m_full = true;
		std::cout << "TimerSaver full - exit program to save file" << std::endl;
	}

}

void TimerSaver::saveToFile(const std::string& filePrefix) {

#ifdef _DEBUG
	return;
#endif

	for (auto& pair : m_results) {
		unsigned int numDifferent = 0;
		
		std::ofstream outFile;
		
		auto t = std::time(nullptr);
		tm mtm;
		localtime_s(&mtm, &t);
		std::ostringstream oss;
		oss << std::put_time(&mtm, "%Y-%m-%d_%H-%M-%S");
		outFile.open(filePrefix + pair.first + "_" + oss.str() + ".tsv", std::ios_base::app);

		// First row
		std::string firstLine = "#\t";
		for (auto& entry : pair.second) {
			firstLine += std::to_string(entry.first);
			firstLine += "\t";
		}
		outFile << firstLine << "\n";
		
		// Data
		for (unsigned int i = 0; i < m_maxResults; i++) {
			std::stringstream ss;
			ss << i+1 << "\t";
			int numMisses = 0;
			for (auto& entry : pair.second) {
				if (entry.second.size() > i)
					ss << entry.second[i];
				else {
					ss << " ";
					numMisses++;
				}
				ss << "\t";
			}
			if (numMisses == pair.second.size())
				break;
			outFile << ss.str() << "\n";
		}

		outFile.close();

	}
}
