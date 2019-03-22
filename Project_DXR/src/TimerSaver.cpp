#include "pch.h"
#include "TimerSaver.h"
#include "DX12/DX12Renderer.h"

#include <iomanip>
#include <time.h>
#include <ctime>
#include <direct.h>

TimerSaver::TimerSaver(unsigned int numResultsToSave, DX12Renderer* renderer)
	: m_maxResults(numResultsToSave)
	, m_full(false)
	, m_renderer(renderer)
{
#ifdef _DEBUG
	std::cout << "RUNNING IN DEBUG - NO TIMERS WILL BE SAVED" << std::endl;
	m_full = true;
#endif
}

TimerSaver::~TimerSaver() {
}

void TimerSaver::addResult(const std::string& name, int column, double valueMs) {

	if (m_results[name][column].size() >= m_maxResults) {
		std::cout << "Error: Tried to add data to full column (" << column << ")." << std::endl;
		return;
	}

	if (m_results[name].find(column) == m_results[name].end()) {
		m_results[name][column].reserve(m_maxResults);
	}
	m_results[name][column].emplace_back(valueMs);

	if (m_results[name][column].size() >= m_maxResults) {
		std::cout << "TimerSaver full at column: " << column << " - exit program to save file" << std::endl;
	}

}

void TimerSaver::saveToFile(const std::string& filePrefix) {

#ifdef _DEBUG
	return;
#endif

	auto t = std::time(nullptr);
	tm mtm;
	localtime_s(&mtm, &t);
	std::ostringstream oss;
	oss << std::put_time(&mtm, "%Y-%m-%d_%H-%M-%S");

	std::string foldername = "" + oss.str();
	//std::string rawDataFoldername = foldername + "/data";
	
	_mkdir(foldername.c_str());
	//_mkdir(rawDataFoldername.c_str());


	for (auto& pair : m_results) {
		unsigned int numDifferent = 0;
		
		std::ofstream outFile;
		
		outFile.open(foldername + "/" + filePrefix + pair.first + ".tsv", std::ios_base::app);

		// First two rows
		std::string firstLine = "#\t";
		std::stringstream ss;
		const DX12Renderer::GPUInfo& gpuInfo = m_renderer->getGPUInfo();
		ss << "GPU: " << gpuInfo.description << "GB, ";
		ss << "Dedicated VRAM: " << gpuInfo.dedicatedVideoMemory << "GB, ";
		ss << "Shared VRAM: " << gpuInfo.sharedSystemMemory << "GB";
		firstLine += ss.str();
		firstLine += "\n#\t";
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

unsigned int TimerSaver::getSizeOf(const std::string& name, int column) {
	return m_results[name][column].size();
}
