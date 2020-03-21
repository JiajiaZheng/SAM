#include <future>

#ifdef __WXMSW__
#include <windows.h>
#endif

#include "pythoninstall.h"

PythonConfig ReadPythonConfig(const std::string& configPath) {
    // load python configuration
    Json::Value python_config_root;
    std::ifstream python_config_doc(configPath);
    if (python_config_doc.fail())
        throw std::runtime_error("Could not open " + configPath );

    python_config_doc >> python_config_root;

    if (!python_config_root.isMember("miniconda_version"))
        throw std::runtime_error("Missing key 'miniconda_version' in " + configPath);
    if (!python_config_root.isMember("python_version"))
        throw std::runtime_error("Missing key 'python_version' in " + configPath);
    if (!python_config_root.isMember("exec_path"))
        throw std::runtime_error("Missing key 'exec_path' in " + configPath);
    if (!python_config_root.isMember("pip_path"))
        throw std::runtime_error("Missing key 'pip_path' in " + configPath);

    PythonConfig config = {python_config_root["python_version"].asString(),
                           python_config_root["miniconda_version"].asString(),
                           python_config_root["exec_path"].asString(),
                           python_config_root["pip_path"].asString()};

    return config;
}

bool CheckPythonInstalled(const PythonConfig& config){
    return !config.execPath.empty() && !config.pipPath.empty();
}

bool external_process_pipe(const std::string& cmd){
    std::promise<int> install_result;
    std::future<int> f_completes = install_result.get_future();
    std::thread([&](std::promise<int> install_result)
                {
                    install_result.set_value_at_thread_exit(system(cmd.c_str()));
                },
                std::move(install_result)
    ).detach();

    std::chrono::system_clock::time_point time_passed
            = std::chrono::system_clock::now() + std::chrono::seconds(60 * 10);

    if(std::future_status::ready == f_completes.wait_until(time_passed))
        return f_completes.get();
    else
        throw std::runtime_error("Python Installation error: process timed out with command: " + cmd);
}

bool InstallPythonWindows(const std::string& path, const PythonConfig& config){
    std::string cmd = path + "/install_python.ps1 -version " + config.pythonVersion + " -config " + path;
//    bool error = external_process_pipe(cmd);
    int rvalue = system(cmd.c_str());
    return (bool)rvalue;
}

bool InstallPythonUnix(const std::string& path, const PythonConfig& config){
    std::string cmd = path + "/install_python.sh " + config.minicondaVersion + " " + config.pythonVersion + " " + path;
    int rvalue = system(cmd.c_str());
    return (bool)rvalue;
}

pythonPackageConfig ReadPythonPackageConfig(const std::string& name, const std::string& configFile){
    // load python configuration
    Json::Value python_config_root;
    std::ifstream python_config_doc(configFile);
    if (python_config_doc.fail())
        throw std::runtime_error("Could not open " + configFile );

    python_config_doc >> python_config_root;

    if (!python_config_root.isMember("min_python_version"))
        throw std::runtime_error("Missing key 'min_python_version' in " + configFile);
    if (!python_config_root.isMember("run_cmd"))
        throw std::runtime_error("Missing key 'run_cmd' in " + configFile);
    if (!python_config_root.isMember("version"))
        throw std::runtime_error("Missing key 'version' in " + configFile);

    pythonPackageConfig config = {name,
                                  python_config_root["min_python_version"].asString(),
                                  python_config_root["run_cmd"].asString(),
                                  python_config_root["version"].asString()};

    return config;
}

bool InstallFromPip(const std::string& pip_exec, const pythonPackageConfig& package){
    std::string cmd = pip_exec + " install " + package.name + "==" + package.version;
    int rvalue = system(cmd.c_str());
    return (bool)rvalue;
}
