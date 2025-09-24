#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include "nlohmann/json.hpp"

std::string GetTexPath(const char* name);
std::string GetDataPath(const char* name);

class AtlasMgr {
public:
	AtlasMgr() = default;
	virtual ~AtlasMgr() {}
	static bool LoadImpl(const char* jsonFn, AtlasMgr* ptr);

protected:
	virtual bool LoadJson(const nlohmann::json& data) = 0;
};

template<class T>
std::unique_ptr<T> CreateAtlasMgr(const char* jsonFn) {
	std::unique_ptr<T> uptr(new T);
	if (AtlasMgr::LoadImpl(jsonFn, uptr.get()))
		return uptr;
	return nullptr;
}

class IconMgr : public AtlasMgr {
public:
	IconMgr() = default;
	~IconMgr() = default;

	const glm::ivec4* GetRect(const char* name) const;

protected:
    bool LoadJson(const nlohmann::json& data) override;

private:
	std::unordered_map<std::string, glm::ivec4> m_Map;
};