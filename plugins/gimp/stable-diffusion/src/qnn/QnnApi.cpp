// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------

#include "QnnApi.hpp"


static void logStdoutCallback(const char* fmt, QnnLog_Level_t level, uint64_t timestamp, va_list argp) {
    const char* levelStr = "";
    switch (level) {
    case QNN_LOG_LEVEL_ERROR:
        levelStr = " ERROR ";
        break;
    case QNN_LOG_LEVEL_WARN:
        levelStr = "WARNING";
        break;
    case QNN_LOG_LEVEL_INFO:
        levelStr = "  INFO ";
        break;
    case QNN_LOG_LEVEL_DEBUG:
        levelStr = " DEBUG ";
        break;
    case QNN_LOG_LEVEL_VERBOSE:
        levelStr = "VERBOSE";
        break;
    case QNN_LOG_LEVEL_MAX:
        levelStr = "UNKNOWN";
        break;
    }

    char str[1000] = "";
    double ms = (double)timestamp / 1000000.0;
    int offset = snprintf(str, sizeof(str), "%8.1fms [%-7s] ", ms, levelStr);
    vsnprintf(str + offset, sizeof(str) - offset, fmt, argp);
    printf("%s", str);
}


QnnApi::~QnnApi()
{
    QNN_DEBUG("Destroying Performance");
    if (true != destroyPerformance()) {
        QNN_DEBUG("Could not destroy Performance");
    }

    QNN_DEBUG("Freeing Graphs");
    if (true != freeGraphs()) {
        QNN_DEBUG("Could not free Graphs");
    }

    // Free context if not already done
    if (m_isContextCreated) {
        QNN_DEBUG("Freeing Context");
        if (true != freeContext()) {
            QNN_DEBUG("Could not free context");
        }
    }

    QNN_DEBUG("Freeing Device");
    if (true != freeDevice()) {
        QNN_ERROR("Device Free failure");
    }

    QNN_DEBUG("Terminating Logging");
    if (m_isLogInitialized) {
        terminateLog();
    }
    m_isLogInitialized = false;

    // Terminate backend
    if (m_isBackendInitialized) {
        QNN_DEBUG("Terminating Backend");
        if (true != terminateBackend()) {
            QNN_DEBUG("Could not terminate backend");
        }
    }

    if (m_backendLibraryHandle)
    {
        QNN_DEBUG("Closing Backend Lib Handle");
        dlclose(m_backendLibraryHandle);
    }

    if (m_libModelHandle)
    {
        QNN_DEBUG("Closing Model Lib Handle");
        dlclose(m_libModelHandle);
    }
}

bool QnnApi::getContextConfigs(QnnContext_Config_t ***configs,
                            uint32_t &contextConfigCount,
                            Qnn_Priority_t contextPriority)
{
    QnnContext_Config_t **contextConfigs{nullptr};
    if (contextPriority != QNN_PRIORITY_DEFAULT) {
        contextConfigCount = 1;
        contextConfigs = (QnnContext_Config_t **)malloc(contextConfigCount * sizeof(QnnContext_Config_t *));
        if (nullptr == contextConfigs) {
            QNN_ERROR("Could not allocate memory for allContextConfigs");
            return false;
        }
        for (size_t i = 0; i < contextConfigCount; i++) {
            contextConfigs[i] = (QnnContext_Config_t *)malloc(sizeof(QnnContext_Config_t));
        }
        contextConfigs[0]->option   = QnnContext_ConfigOption_t::QNN_CONTEXT_CONFIG_OPTION_PRIORITY;
        contextConfigs[0]->priority = contextPriority;
    }
    *configs = contextConfigs;

    return true;
}

bool QnnApi::mergeAllContextConfigs(
    QnnContext_Config_t ***allCustomContextConfigs,
    QnnContext_Config_t **customConfigs,
    QnnContext_Config_t **contextConfigs,
    uint32_t customConfigCount,
    uint32_t contextConfigCount)
{
    QnnContext_Config_t **allContextConfigs{nullptr};
    if (contextConfigCount + customConfigCount > 0) {
        allContextConfigs = (QnnContext_Config_t **)calloc((contextConfigCount + customConfigCount + 1),
                                                            sizeof(QnnContext_Config_t *));
        if (nullptr == allContextConfigs) {
            QNN_ERROR("Could not allocate memory for allContextConfigs");
            return false;
        }
        for (size_t cnt = 0; cnt < contextConfigCount; cnt++) {
            allContextConfigs[cnt] = contextConfigs[cnt];
        }
        for (size_t cnt = 0; cnt < customConfigCount; cnt++) {
            allContextConfigs[cnt + contextConfigCount] = customConfigs[cnt];
        }
    }
    *allCustomContextConfigs = allContextConfigs;

    return true;
}

bool QnnApi::freeContextConfigs(QnnContext_Config_t **contextConfigs,
                                uint32_t contextConfigCount)
{
    if (contextConfigs) {
        for (size_t i = 0; i < contextConfigCount; i++) {
            free(contextConfigs[i]);
        }
        free(contextConfigs);
    }

    return true;
}

bool QnnApi::setGraphConfigsBeforeExecute(
    Qnn_GraphHandle_t graphHandle,
    QnnGraph_Config_t **graphConfigs,
    uint32_t configCount)
{
    if (!graphConfigs || configCount == 0u) {
        QNN_ERROR("No graph configs to set");
        return false;
    }

#ifdef QNN_ENABLE_API_2x
    std::vector<const QnnGraph_Config_t *> graphConfigsPointers(configCount + 1, nullptr);
    for (size_t idx = 0u; idx < configCount; idx++) {
        graphConfigsPointers[idx] = graphConfigs[idx];
    }
    if (QNN_SUCCESS != m_qnnInterface.graphSetConfig(graphHandle, graphConfigsPointers.data())) {
        QNN_ERROR("Failed to set graph configs.");
        return false;
    }
#else
    (void)graphHandle;
    (void)graphConfigs;
    (void)configCount;
#endif

    return true;
}

bool QnnApi::getQnnInterface(std::string backendPath)
{
#ifdef UWP
    m_backendLibraryHandle = LoadLibraryExW(std::wstring(backendPath.begin(), backendPath.end()).c_str(), NULL, 0);
#else
    m_backendLibraryHandle = dlopen(backendPath.c_str(), RTLD_NOW);
#endif    
    if (nullptr == m_backendLibraryHandle) {
        QNN_ERROR("Unable to load backend. dlerror(): %s", dlerror());
        return false;
    }

    // Get QNN Interface
    QnnInterfaceGetProvidersFn_t getInterfaceProviders{nullptr};
#ifdef UWP
    getInterfaceProviders = (QnnInterfaceGetProvidersFn_t)GetProcAddress((HMODULE)m_backendLibraryHandle, "QnnInterface_getProviders");
#else
    getInterfaceProviders = (QnnInterfaceGetProvidersFn_t)dlsym(m_backendLibraryHandle, "QnnInterface_getProviders");
#endif
    
    if (nullptr == getInterfaceProviders) {
        return false;
    }

    uint32_t numProviders{0};
#ifdef QNN_ENABLE_API_2x_P3
    QnnInterface_t** interfaceProviders{nullptr};
    if (QNN_SUCCESS != getInterfaceProviders((const QnnInterface_t***)&interfaceProviders, &numProviders)) {
        QNN_ERROR("Failed to get interface providers.");
        return false;
    }
#else
    QnnInterface_t* interfaceProviders{nullptr};
    if (QNN_SUCCESS != getInterfaceProviders((const QnnInterface_t**)&interfaceProviders, &numProviders)) {
        QNN_ERROR("Failed to get interface providers.");
        return false;
    }
#endif

    if (nullptr == interfaceProviders) {
        QNN_ERROR("Failed to get interface providers: null interface providers received.");
        return false;
    }
    if (0u == numProviders) {
        QNN_ERROR("Failed to get interface providers: 0 interface providers.");
        return false;
    }

    bool foundValidInterface{false};
    for (size_t pIdx = 0; pIdx < numProviders; pIdx++) {
#ifdef QNN_ENABLE_API_2x_P3
        const Qnn_ApiVersion_t& apiVersion = interfaceProviders[pIdx]->apiVersion;
#else
        const Qnn_ApiVersion_t& apiVersion = interfaceProviders[pIdx].apiVersion;
#endif
        if ((QNN_API_VERSION_MAJOR == apiVersion.coreApiVersion.major) &&
            (QNN_API_VERSION_MINOR <= apiVersion.coreApiVersion.minor)) {
            foundValidInterface = true;
#ifdef QNN_ENABLE_API_2x_P3
            m_qnnInterface = interfaceProviders[pIdx]->QNN_INTERFACE_VER_NAME;
#else
            m_qnnInterface = interfaceProviders[pIdx].QNN_INTERFACE_VER_NAME;
#endif
            break;
        }
    }
    if (!foundValidInterface) {
        QNN_ERROR("Unable to find a valid interface.");
        m_backendLibraryHandle = nullptr;
        return false;
    }

    return true;
}

bool QnnApi::getQnnSystemInterface(std::string systemLibraryPath)
{
#ifdef UWP
    void* systemLibraryHandle = LoadLibraryExW(std::wstring(systemLibraryPath.begin(), systemLibraryPath.end()).c_str(), NULL, 0);
#else
    void* systemLibraryHandle = dlopen(systemLibraryPath.c_str(), RTLD_NOW);
#endif
    if (nullptr == systemLibraryHandle) {
        QNN_ERROR("Unable to load system library. dlerror(): %s", dlerror());
        return false;
    }

    // Get QNN System Interface
    QnnSystemInterfaceGetProvidersFn_t getSystemInterfaceProviders{nullptr};
#ifdef UWP
    getSystemInterfaceProviders = (QnnSystemInterfaceGetProvidersFn_t)GetProcAddress((HMODULE)systemLibraryHandle, "QnnSystemInterface_getProviders");
#else
    getSystemInterfaceProviders = (QnnSystemInterfaceGetProvidersFn_t) dlsym(
                                        systemLibraryHandle, "QnnSystemInterface_getProviders");
#endif
    if (nullptr == getSystemInterfaceProviders) {
        return false;
    }
    uint32_t numProviders{0};
#ifdef QNN_ENABLE_API_2x_P3
    QnnSystemInterface_t** systemInterfaceProviders{nullptr};
    if (QNN_SUCCESS != getSystemInterfaceProviders(
                            (const QnnSystemInterface_t***)&systemInterfaceProviders, &numProviders)) {
        QNN_ERROR("Failed to get system interface providers.");
        return false;
    }
#else
    QnnSystemInterface_t* systemInterfaceProviders{nullptr};
    if (QNN_SUCCESS != getSystemInterfaceProviders(
                            (const QnnSystemInterface_t**)&systemInterfaceProviders, &numProviders)) {
        QNN_ERROR("Failed to get system interface providers.");
        return false;
    }
#endif
    if (nullptr == systemInterfaceProviders) {
        QNN_ERROR("Failed to get system interface providers: null system interface providers received.");
        return false;
    }
    if (0 == numProviders) {
        QNN_ERROR("Failed to get system interface providers: 0 system interface providers.");
        return false;
    }

    bool foundValidSystemInterface{false};
    for (size_t pIdx = 0; pIdx < numProviders; pIdx++) {
#ifdef QNN_ENABLE_API_2x_P3
        const Qnn_Version_t& systemApiVersion = systemInterfaceProviders[pIdx]->systemApiVersion;
#else
        const Qnn_Version_t& systemApiVersion = systemInterfaceProviders[pIdx].systemApiVersion;
#endif
        if (QNN_SYSTEM_API_VERSION_MAJOR == systemApiVersion.major &&
            QNN_SYSTEM_API_VERSION_MINOR <= systemApiVersion.minor) {
            foundValidSystemInterface = true;
#ifdef QNN_ENABLE_API_2x_P3
            m_qnnSystemInterface = systemInterfaceProviders[pIdx]->QNN_SYSTEM_INTERFACE_VER_NAME;
#else
            m_qnnSystemInterface = systemInterfaceProviders[pIdx].QNN_SYSTEM_INTERFACE_VER_NAME;
#endif
            break;
        }
    }
    if (!foundValidSystemInterface) {
        QNN_ERROR("Unable to find a valid system interface.");
        return false;
    }

    return true;
}

bool QnnApi::loadModel(std::string model_path)
{
    const char *dlsym_error;

    dlerror();    
#ifdef UWP
    m_libModelHandle = LoadLibraryExW(std::wstring(model_path.begin(), model_path.end()).c_str(), NULL, 0);
#else
    m_libModelHandle = dlopen(model_path.c_str(), RTLD_NOW);
#endif // UWP
    
    if (nullptr == m_libModelHandle) {
        QNN_ERROR("Unable to load model. dlerror(): %s", dlerror());
        return false;
    }

    // Currently model Prefix is fixed. If model was prepared with
    // custom prefix, we need to change this.
    std::string modelPrefix = "QnnModel";

    std::string modelPrepareFunc = modelPrefix+"_composeGraphs";

#ifdef UWP
    m_composeGraphsFnHandle = (ComposeGraphsFnHandleType_t)GetProcAddress((HMODULE)m_libModelHandle, modelPrepareFunc.c_str());
#else
    m_composeGraphsFnHandle = (ComposeGraphsFnHandleType_t)dlsym(m_libModelHandle, modelPrepareFunc.c_str());
#endif
    
    dlsym_error = dlerror();
    if (dlsym_error || nullptr == m_composeGraphsFnHandle) {
        QNN_ERROR("Did not find QnnModel_composeGraphs function: %s", dlsym_error);
        return false;
    }

    std::string modelFreeFunc = modelPrefix+"_freeGraphsInfo";
#ifdef UWP
    m_freeGraphInfoFnHandle = (FreeGraphInfoFnHandleType_t)GetProcAddress((HMODULE)m_libModelHandle, modelFreeFunc.c_str());
#else
    m_freeGraphInfoFnHandle = (FreeGraphInfoFnHandleType_t)dlsym(m_libModelHandle, modelFreeFunc.c_str());
#endif
    
    dlsym_error = dlerror();
    if (dlsym_error || nullptr == m_freeGraphInfoFnHandle) {
        QNN_ERROR("Did not find QnnModel_freeGraphsInfo function: %s", dlsym_error);
        return false;
    }

    return true;
}




bool QnnApi::initializeLogging(const QnnLog_Level_t &logLevel) {
    // initialize logging in the backend
#ifdef QNN_ENABLE_API_2x_P2
    if (nullptr != m_qnnInterface.logCreate) {
        auto logCallback = logStdoutCallback;
        QNN_DEBUG("Initializing logging in the backend. Callback: [%p], Log Level: [%d]",
                    logCallback,
                    logLevel);
        if (QNN_SUCCESS != m_qnnInterface.logCreate(logCallback, logLevel, &m_logHandle)) {
            QNN_WARN("Unable to initialize logging in the backend.");
        }
        m_isLogInitialized = true;
    }
#else
    if (nullptr != m_qnnInterface.logInitialize) {
        auto logCallback = nullptr;
        QNN_DEBUG("Initializing logging in the backend. Callback: [%p], Log Level: [%d]",
                    logCallback,
                    logLevel);
        if (QNN_SUCCESS != m_qnnInterface.logInitialize(logCallback, logLevel)) {
            QNN_WARN("Unable to initialize logging in the backend.");
        }
        m_isLogInitialized = true;
    }
#endif
    else {
        QNN_WARN("Logging not available in the backend.");
        return true;
    }

    return true;
}

void QnnApi::terminateLog() {
    // Terminate logging in the backend
#ifdef QNN_ENABLE_API_2x_P2
    if (nullptr != m_qnnInterface.logFree && nullptr != m_logHandle) {
        if (QNN_SUCCESS != m_qnnInterface.logFree(m_logHandle)) {
            QNN_WARN("Unable to terminate logging in the backend.");
        }
    }
#else
    if (nullptr != m_qnnInterface.logTerminate) {
        if (QNN_SUCCESS != m_qnnInterface.logTerminate()) {
            QNN_WARN("Unable to terminate logging in the backend.");
        }
    }
#endif
}

bool QnnApi::initializeBackendExtensions(BackendExtensionsConfigs backendExtensionsConfig,
                                         PerfProfile parsedPerfProfile)
{

    std::unique_ptr<BackendExtensions> backendExtensions(
        new BackendExtensions(backendExtensionsConfig,
                              m_backendLibraryHandle, parsedPerfProfile));
    if (nullptr == backendExtensions) {
        QNN_ERROR("Unable to create backend extensions object.");
        return false;
    }
    if (!backendExtensions->initialize()) {
        QNN_ERROR("Unable to initialize backend extensions.");
        return false;
    }
    m_backendExtensions = std::move(backendExtensions);

    return true;
}

// Initialize a QnnBackend.
bool QnnApi::initializeBackend()
{
#ifdef QNN_ENABLE_API_2x_P2
    if (nullptr == m_qnnInterface.backendCreate) {
        QNN_ERROR("BackendCreate API is not supported for this backend");
        return false;
    }
#else
    if (nullptr == m_qnnInterface.backendInitialize) {
        QNN_ERROR("BackendInitialize API is not supported for this backend");
        return false;
    }
#endif

    QnnBackend_Config_t** customConfigs{nullptr};
    uint32_t customConfigCount{0};
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeBackendInitialize(&customConfigs,
                                                                       &customConfigCount)) {
            QNN_ERROR("Extensions Failure in beforeBackendInitialize()");
            return false;
        }
    }
    QnnBackend_Config_t** allBackendConfigs{nullptr};
    if ((m_backendConfigCount + customConfigCount) > 0) {
        allBackendConfigs = (QnnBackend_Config_t**)calloc(
            (m_backendConfigCount + customConfigCount + 1), sizeof(QnnBackend_Config_t*));
        if (nullptr == allBackendConfigs) {
            QNN_ERROR("Could not allocate memory for allBackendConfigs");
            return false;
        }
        for (size_t cnt = 0; cnt < m_backendConfigCount; cnt++) {
            allBackendConfigs[cnt] = m_backendConfigs[cnt];
        }
        for (size_t cnt = 0; cnt < customConfigCount; cnt++) {
            allBackendConfigs[cnt + m_backendConfigCount] = customConfigs[cnt];
        }
    }

#ifdef QNN_ENABLE_API_2x_P2
    auto returnStatus = m_qnnInterface.backendCreate(
                m_logHandle, (const QnnBackend_Config_t **)allBackendConfigs, &m_backendHandle);
    if (QNN_SUCCESS != returnStatus) {
        QNN_ERROR("Could not initialize backend due to error = %llu", (unsigned long long)returnStatus);
        if (allBackendConfigs) {
            free(allBackendConfigs);
        }
        return false;
    }
#else
    auto returnStatus = m_qnnInterface.backendInitialize(
                                            (const QnnBackend_Config_t **)allBackendConfigs);
    if (QNN_BACKEND_NO_ERROR != returnStatus && QNN_BACKEND_ERROR_ALREADY_INITIALIZED != returnStatus) {
        QNN_ERROR("Could not initialize backend due to error = %llu", (unsigned long long)returnStatus);
        if (allBackendConfigs) {
            free(allBackendConfigs);
        }
        return false;
    }
    if (QNN_BACKEND_ERROR_ALREADY_INITIALIZED == returnStatus) {
        QNN_WARN("A backend is already initialized and the error is ignored");
    }
#endif
    QNN_DEBUG("Initialize Backend Returned Status = %llu", (unsigned long long)returnStatus);

    m_isBackendInitialized = true;
    if (allBackendConfigs) {
        free(allBackendConfigs);
    }

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterBackendInitialize()) {
            QNN_ERROR("Extensions Failure in afterBackendInitialize()");
            return false;
        }
    }

    return true;
}

// Terminate the backend after done.
bool QnnApi::terminateBackend()
{

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeBackendTerminate()) {
            QNN_ERROR("Extensions Failure in beforeBackendTerminate()");
            return false;
        }
    }
    // Terminate backend
#ifdef QNN_ENABLE_API_2x_P2
    if (m_isBackendInitialized && nullptr != m_qnnInterface.backendFree) {
        QNN_DEBUG("Freeing backend");
        if (QNN_BACKEND_NO_ERROR != m_qnnInterface.backendFree(m_backendHandle)) {
            QNN_ERROR("Could not free backend");
        }
    }
#else
    if (m_isBackendInitialized && nullptr != m_qnnInterface.backendTerminate) {
        QNN_DEBUG("Terminating backend");
        if (QNN_BACKEND_NO_ERROR != m_qnnInterface.backendTerminate()) {
            QNN_ERROR("Could not terminate backend");
        }
    }
#endif
    m_isBackendInitialized = false;

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterBackendTerminate()) {
            QNN_ERROR("Extensions Failure in afterBackendTerminate()");
            return false;
        }
    }

    return true;
}

bool QnnApi::createDevice()
{
#ifdef QNN_ENABLE_API_2x_P2
    QnnDevice_Config_t **deviceConfigs{nullptr};
    uint32_t configCount{0};
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeCreateDevice(&deviceConfigs, &configCount)) {
            QNN_ERROR("Extensions Failure in beforeCreateDevice()");
            return false;
        }
    }
    std::vector<const QnnDevice_Config_t *> deviceConfigPointers(configCount + 1, nullptr);
    for (size_t idx = 0u; idx < configCount; idx++) {
        deviceConfigPointers[idx] = deviceConfigs[idx];
    }
    if (nullptr != m_qnnInterface.deviceCreate) {
        auto qnnStatus =
            m_qnnInterface.deviceCreate(m_logHandle, deviceConfigPointers.data(), &m_deviceHandle);
        if (QNN_SUCCESS != qnnStatus) {
            if (QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE == qnnStatus) {
                QNN_WARN("Device feature unsupported");
            } else {
                QNN_ERROR("Failed to create device: %lu", qnnStatus);
                return false;
            }
        }
    }
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterCreateDevice()) {
            QNN_ERROR("Extensions Failure in afterCreateDevice()");
            return false;
        }
    }
#endif
    return true;
}

bool QnnApi::freeDevice() {
#ifdef QNN_ENABLE_API_2x_P2
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeFreeDevice()) {
            QNN_ERROR("Extensions Failure in beforeFreeDevice()");
            return false;
        }
    }
    if (nullptr != m_qnnInterface.deviceFree) {
        auto qnnStatus = m_qnnInterface.deviceFree(m_deviceHandle);
        if (QNN_SUCCESS != qnnStatus) {
            if (QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE != qnnStatus) {
                QNN_WARN("Device feature unsupported");
            } else {
                QNN_ERROR("Failed to free device: %lu", qnnStatus);
                return false;
            }
        }
    }
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterFreeDevice()) {
            QNN_ERROR("Extensions Failure in afterfreeDevice()");
            return false;
        }
    }
#endif
    return true;
}

// Create a Context in a backend.
bool QnnApi::createContext(ContextConfigs contextConfig)
{
    QnnContext_Config_t** customConfigs{nullptr};
    uint32_t customConfigCount{0};
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeContextCreate(&customConfigs,
                                                                   &customConfigCount)) {
            QNN_ERROR("Extensions Failure in beforeContextCreate()");
            return false;
        }
    }

    QnnContext_Config_t **contextConfigs = nullptr;
    uint32_t contextConfigCount          = 0;
    if (true != getContextConfigs(&contextConfigs, contextConfigCount, contextConfig.priority)) {
        QNN_ERROR("Couldn't populate context configs");
        return false;
    }

    QnnContext_Config_t **allContextConfigs{nullptr};
    if (true != mergeAllContextConfigs(&allContextConfigs,
                                       customConfigs,
                                       contextConfigs,
                                       customConfigCount,
                                       contextConfigCount)) {
        QNN_ERROR("Error merging custom and context configs");
        return false;
    }

    Qnn_ContextHandle_t contextHandle{nullptr};
    if (QNN_CONTEXT_NO_ERROR != m_qnnInterface.contextCreate(
#ifdef QNN_ENABLE_API_2x_P2
                  m_backendHandle,
                  nullptr,
#endif            
                  (const QnnContext_Config_t**)allContextConfigs, &contextHandle)) {
        QNN_ERROR("Could not create context");
        if (allContextConfigs) {
            free(allContextConfigs);
        }

        return false;
    }

    m_contextVec.push_back(contextHandle);
    m_isContextCreated = true;
    if (allContextConfigs) {
        free(allContextConfigs);
    }

    if (true != freeContextConfigs(contextConfigs, contextConfigCount)) {
        QNN_ERROR("Couldn't free context configs");
        return false;
    }

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterContextCreate()) {
            QNN_ERROR("Extensions Failure in afterContextCreate()");
            return false;
        }
    }

    return true;
}

// Free context after done.
bool QnnApi::freeContext()
{

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeContextFree()) {
            QNN_ERROR("Extensions Failure in beforeContextFree()");
            return false;
        }
    }
    for (const auto& context : m_contextVec) {
        if (QNN_CONTEXT_NO_ERROR != m_qnnInterface.contextFree(context, nullptr)) {
            QNN_ERROR("Could not free context");
            return false;
        }
    }
    m_isContextCreated = false;

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterContextFree()) {
            QNN_ERROR("Extensions Failure in afterContextFree()");
            return false;
        }
    }

    return true;
}

// Calls composeGraph function in QNN's model.so.
// composeGraphs is supposed to populate graph related
// information in graphsInfo and graphsCount.
// m_debug is the option supplied to composeGraphs to
// say that all intermediate tensors including output tensors
// are expected to be read by the app.
bool QnnApi::composeGraphs(std::vector<GraphConfigs> graphConfigs)
{
    GraphConfigInfo_t** customConfigs{nullptr};
    uint32_t customConfigGraphsCount{0};
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeComposeGraphs(&customConfigs,
                                                                   &customConfigGraphsCount)) {
            QNN_ERROR("Extensions Failure in beforeComposeGraphs()");
            return false;
        }
    }

    std::map<std::string, std::vector<QnnGraph_Config_t *>> graphConfigsPointers;
    if (!graphConfigs.empty()) {
        for (auto const& inputGraphConfig : graphConfigs) {
            // Only reset the memory for this graph, if it has not previously been populated with
            // something
            if (graphConfigsPointers.find(inputGraphConfig.graphName) == graphConfigsPointers.end()) {
                graphConfigsPointers[inputGraphConfig.graphName] = std::vector<QnnGraph_Config_t*>();
                graphConfigsPointers[inputGraphConfig.graphName].reserve(s_graphConfigsReserveCount);
            }
#ifndef QNN_ENABLE_API_2x_P3
            if (inputGraphConfig.asyncExecutionOrderPresent) {
                QnnGraph_Config_t* newGraphConfig = (QnnGraph_Config_t*)malloc(sizeof(QnnGraph_Config_t));
                newGraphConfig->option            = QNN_GRAPH_CONFIG_OPTION_ASYNC_EXECUTION_ORDER;
                newGraphConfig->asyncExeOrder     = inputGraphConfig.asyncExecutionOrder;
                graphConfigsPointers[inputGraphConfig.graphName].push_back(newGraphConfig);
            }
            if (inputGraphConfig.asyncExeQueueDepthPresent) {
                QnnGraph_Config_t* newGraphConfig  = (QnnGraph_Config_t*)malloc(sizeof(QnnGraph_Config_t));
                newGraphConfig->option             = QNN_GRAPH_CONFIG_OPTION_ASYNC_EXECUTION_QUEUE_DEPTH;
                newGraphConfig->asyncExeQueueDepth = inputGraphConfig.asyncExeQueueDepth;
                graphConfigsPointers[inputGraphConfig.graphName].push_back(newGraphConfig);
            }
#endif  // QNN_ENABLE_API_2x_P3
#ifdef QNN_ENABLE_API_2x
            if (inputGraphConfig.priorityPresent) {
                QnnGraph_Config_t* newGraphConfig = (QnnGraph_Config_t*)malloc(sizeof(QnnGraph_Config_t));
                newGraphConfig->option            = QNN_GRAPH_CONFIG_OPTION_PRIORITY;
                newGraphConfig->priority          = inputGraphConfig.priority;
                graphConfigsPointers[inputGraphConfig.graphName].push_back(newGraphConfig);
            }
#endif
        }
    }

    if (customConfigs != nullptr && customConfigGraphsCount > 0) {
        for (size_t gIdx = 0; gIdx < customConfigGraphsCount; gIdx++) {
            auto configPtr = customConfigs[gIdx]->graphConfigs;
            if (*configPtr &&
                    (!customConfigs[gIdx]->graphName || strlen(customConfigs[gIdx]->graphName) == 0)) {
                QNN_ERROR("Graph configs specified without a graph name in the backend extensions.");
                return false;
            }
            if (customConfigs[gIdx]->graphName && strlen(customConfigs[gIdx]->graphName) > 0 &&
                    *configPtr) {
                if (graphConfigsPointers.find(customConfigs[gIdx]->graphName) ==
                        graphConfigsPointers.end()) {
                    graphConfigsPointers[customConfigs[gIdx]->graphName] =
                        std::vector<QnnGraph_Config_t*>();
                    graphConfigsPointers[customConfigs[gIdx]->graphName].reserve(
                        s_graphConfigsReserveCount);
                }
                while (*configPtr) {
                    graphConfigsPointers[customConfigs[gIdx]->graphName].push_back(
                        (QnnGraph_Config_t*)*configPtr);
                    configPtr++;
                }
            }
        }
    }

    GraphConfigInfo_t** graphConfigsInfo{nullptr};
    graphConfigsInfo = (GraphConfigInfo_t**)calloc(
                            graphConfigsPointers.size(), sizeof(GraphConfigInfo_t*));
    size_t graphIdx{0};
    for (auto const& graphConfig : graphConfigsPointers) {
        if (graphConfigsInfo && graphConfig.second.size() > 0) {
            graphConfigsInfo[graphIdx] =
                (GraphConfigInfo_t*)malloc(sizeof(GraphConfigInfo_t));
            graphConfigsInfo[graphIdx]->graphName    = (char*)graphConfig.first.c_str();
            graphConfigsInfo[graphIdx]->graphConfigs = (const QnnGraph_Config_t**)calloc(
                graphConfig.second.size() + 1, sizeof(QnnGraph_Config_t*));
            for (size_t cnt = 0; cnt < graphConfig.second.size(); cnt++) {
                graphConfigsInfo[graphIdx]->graphConfigs[cnt] = graphConfig.second[cnt];
            }
        }
        graphIdx++;
    }

    int status = m_composeGraphsFnHandle(
#ifdef QNN_ENABLE_API_2x_P2
            m_backendHandle,
            m_qnnInterface,
#else
            m_backendLibraryHandle,
#endif
            m_contextVec[0],
            (const GraphConfigInfo_t**)graphConfigsInfo,
            graphConfigsPointers.size(),
            &m_graphsInfo,
            &m_graphsCount,
#ifdef QNN_ENABLE_API_2x_P2
            m_DebugModeRequested,
            nullptr,
            QnnLog_Level_t::QNN_LOG_LEVEL_ERROR
#else
            m_DebugModeRequested
#endif
        );

    if (graphConfigsInfo) {
        for (size_t gIdx = 0; gIdx < graphConfigsPointers.size(); gIdx++) {
            if (graphConfigsInfo[gIdx]) {
                if (graphConfigsInfo[gIdx]->graphConfigs) {
                    free(graphConfigsInfo[gIdx]->graphConfigs);
                    graphConfigsInfo[gIdx]->graphConfigs = nullptr;
                    graphConfigsInfo[gIdx]->graphName    = nullptr;
                }
                free(graphConfigsInfo[gIdx]);
                graphConfigsInfo[gIdx] = nullptr;
            }
        }
        free(graphConfigsInfo);
    }

    for (auto const& graphConfig : graphConfigsPointers) {
        for (size_t cnt = 0; cnt < graphConfig.second.size(); cnt++) {
            if (graphConfig.second[cnt]) {
                free(graphConfig.second[cnt]);
            }
        }
        // graphConfig.second.clear();
    }

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterComposeGraphs()) {
            QNN_ERROR("Extensions Failure in afterComposeGraphs()");
            return false;
        }
    }

    if (0 != status) {
        QNN_ERROR("Failed in composeGraphs()");
        return false;
    }

    // For now, we only handle 1 graph for this framework.
    if (m_graphsCount != 1)
    {
       QNN_ERROR("Only one graph is supported by framework");
       return false;
    }

    return true;
}

bool QnnApi::finalizeGraphs()
{
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeGraphFinalize()) {
            QNN_ERROR("Extensions Failure in beforeGraphFinalize()");
            return false;
        }
    }

    for (size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++) {
        if (QNN_GRAPH_NO_ERROR != m_qnnInterface.graphFinalize(m_graphsInfo[graphIdx]->graph, nullptr, nullptr)) {
            return false;
        }
    }

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterGraphFinalize()) {
            QNN_ERROR("Extensions Failure in afterGraphFinalize()");
            return false;
        }
    }

    return true;
}

bool QnnApi::freeGraphs() {
    auto returnStatus = freeGraphsInfo(&m_graphsInfo, m_graphsCount);
    if (m_graphsInfo) {
        free(m_graphsInfo);
    }
    m_graphsInfo  = nullptr;
    m_graphsCount = 0;

    return true;
}

bool QnnApi::createFromBinary(std::vector<std::string> cachedBinariesPathVec,
                              ContextConfigs contextConfig)
{
    // Let backendExtensions populate configs
    QnnContext_Config_t **customConfigs{nullptr};
    uint32_t customConfigCount{0};
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeCreateFromBinary(&customConfigs,
                                                                      &customConfigCount)) {
            QNN_ERROR("Extensions Failure in beforeCreateFromBinary()");
            return false;
        }
    }

    QnnContext_Config_t **contextConfigs = nullptr;
    uint32_t contextConfigCount          = 0;
    if (true != getContextConfigs(&contextConfigs, contextConfigCount, contextConfig.priority)) {
        QNN_ERROR("Couldn't populate context configs");
        return false;
    }

    // Merge BE specific and agnostic configs
    QnnContext_Config_t **allContextConfigs{nullptr};
    if (true != mergeAllContextConfigs(&allContextConfigs,
                                       customConfigs,
                                       contextConfigs,
                                       customConfigCount,
                                       contextConfigCount)) {
        QNN_ERROR("Error merging custom and context configs");
        return false;
    }

    if (nullptr == m_qnnSystemInterface.systemContextCreate ||
            nullptr == m_qnnSystemInterface.systemContextGetBinaryInfo ||
            nullptr == m_qnnSystemInterface.systemContextFree) {
        QNN_ERROR("QNN System function pointers are not populated.");
        return false;
    }

    // Only 1 graph per context is supported by this framework. So total num of
    // graphs will be equal to num of context files.
    m_graphsCount = cachedBinariesPathVec.size();
    m_graphsInfo  = (GraphInfo_t **)calloc(m_graphsCount, sizeof(GraphInfo_t *));
    for (size_t contextIdx = 0; contextIdx < cachedBinariesPathVec.size(); contextIdx++) {
        uint64_t bufferSize{0};
        std::shared_ptr<uint8_t> buffer{nullptr};
        uint32_t graphsCount;

#ifdef QNN_ENABLE_API_2x
        // read serialized binary into a byte buffer
        bufferSize = getFileSize(cachedBinariesPathVec[contextIdx]);
        if (0 == bufferSize) {
            QNN_ERROR("Received path to an empty file for context index = %zu. Nothing to deserialize.",
                contextIdx);
            return false;
        }

        buffer = std::shared_ptr<uint8_t>(new uint8_t[bufferSize], std::default_delete<uint8_t[]>());
        if (!buffer) {
            QNN_ERROR("Failed to allocate memory for context index = %zu", contextIdx);
            return false;
        }
        if (true != readBinaryFromFile(
                cachedBinariesPathVec[contextIdx], reinterpret_cast<uint8_t *>(buffer.get()), bufferSize)) {
            QNN_ERROR("Failed to read binary data for context index = %zu", contextIdx);
            return false;
        }
#else
        std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>> graphTensorIdToNamesMap;
        if (true != deserializeData(
                        cachedBinariesPathVec[contextIdx], graphTensorIdToNamesMap, &graphsCount,
                        buffer, bufferSize)) {
            QNN_ERROR("Could not deserialize binary file for context index = %zu", contextIdx);
            return false;
        }
#endif

        // inspect binary info
        QnnSystemContext_Handle_t sysCtxHandle{nullptr};
        if (QNN_SUCCESS != m_qnnSystemInterface.systemContextCreate(&sysCtxHandle)) {
            QNN_ERROR("Could not create system handle for context index = %zu", contextIdx);
            return false;
        }

#ifdef QNN_ENABLE_API_2x_P3
        const QnnSystemContext_BinaryInfo_t *binaryInfo{nullptr};
#else
        QnnSystemContext_BinaryInfo_t *binaryInfo{nullptr};
#endif
#ifdef QNN_ENABLE_API_2x
        Qnn_ContextBinarySize_t binaryInfoSize{0};
#else
        uint32_t binaryInfoSize{0};
#endif
        if (QNN_SUCCESS != m_qnnSystemInterface.systemContextGetBinaryInfo(
                                sysCtxHandle,
                                static_cast<void*>(buffer.get()),
                                bufferSize,
                                &binaryInfo,
                                &binaryInfoSize)) {
            QNN_ERROR("Failed to get context binary info for context index = %zu", contextIdx);
            return false;
        }

        GraphInfo_t **graphsInfo;
        if (!copyMetadataToGraphsInfo(binaryInfo, graphsInfo, graphsCount)) {
            QNN_ERROR("Failed to copy metadata for graph index = %zu", contextIdx);
            freeGraphsInfo(&graphsInfo, graphsCount);
            if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
            return false;
        }

        // For now, we only handle 1 graph for this framework.
        if (graphsCount != 1)
        {
           QNN_ERROR("Only one graph per context file is supported by framework.\
                      Found %d graphs for context index = %zu", graphsCount, contextIdx);
           freeGraphsInfo(&graphsInfo, graphsCount);
           if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
           return false;
        }

        m_qnnSystemInterface.systemContextFree(sysCtxHandle);
        sysCtxHandle = nullptr;

#ifndef QNN_ENABLE_API_2x
        if (!populateTensorNamesFromMetadata(graphTensorIdToNamesMap, graphsInfo, graphsCount)) {
            QNN_ERROR("Failed to populate tensor names from metadata for context index = %zu", contextIdx);
            freeGraphsInfo(&graphsInfo, graphsCount);
            if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
            return false;
        }
#endif

        if (nullptr == m_qnnInterface.contextCreateFromBinary) {
            QNN_ERROR("contextCreateFromBinaryFnHandle is nullptr for context index = %zu", contextIdx);
            freeGraphsInfo(&graphsInfo, graphsCount);
            if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
            return false;
        }
        Qnn_ContextHandle_t contextHandle{nullptr};

#ifndef QNN_ENABLE_API_2x
        // auto errCode = m_qnnInterface.contextCreateFromBinaryWithConfig(
        //                         (const QnnContext_Config_t **)allContextConfigs,
        //                         static_cast<void *>(buffer.get()),
        //                         bufferSize,
        //                         &contextHandle,
        //                         nullptr);
        auto errCode = QNN_CONTEXT_ERROR_UNSUPPORTED_FEATURE;
        if (errCode) {
            if (QNN_CONTEXT_ERROR_UNSUPPORTED_FEATURE == errCode) {
                QNN_WARN(
                    "contextCreateFromBinaryWithConfig unsupported on backend. Falling back to "
                    "contextCreateFromBinary for context index = %zu", contextIdx);
                auto retErrCode = m_qnnInterface.contextCreateFromBinary(
                    static_cast<void*>(buffer.get()), bufferSize, &contextHandle, nullptr);
                if (retErrCode) {
                    QNN_ERROR("Could not create context from binary for context index = %zu", contextIdx);
                    freeGraphsInfo(&graphsInfo, graphsCount);
                    if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
                    return false;
                }
            } else {
                QNN_ERROR("Could not create context from binary for context index = %zu", contextIdx);
                freeGraphsInfo(&graphsInfo, graphsCount);
                if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
                return false;
            }
        }
#else
        if (QNN_SUCCESS != m_qnnInterface.contextCreateFromBinary(
#ifdef QNN_ENABLE_API_2x_P2
                                m_backendHandle,
                                nullptr,
#ifdef QNN_ENABLE_API_2x_P3
                                (const QnnContext_Config_t **)allContextConfigs,
#endif
#endif
                                static_cast<void *>(buffer.get()),
                                bufferSize,
                                &contextHandle,
                                nullptr)) {
            QNN_ERROR("Could not create context from binary for context index = %zu", contextIdx);
            freeGraphsInfo(&graphsInfo, graphsCount);
            if (contextIdx > 0)   freeGraphsInfo(&m_graphsInfo, contextIdx-1);
            return false;
        }
#endif

        m_graphsInfo[contextIdx] = graphsInfo[0];
        m_contextVec.push_back(contextHandle);
    }

    m_isContextCreated = true;
    for (size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++) {
        if (nullptr == m_qnnInterface.graphRetrieve) {
            QNN_ERROR("graphRetrieveFnHandle is nullptr.");
            freeGraphsInfo(&m_graphsInfo, m_graphsCount);
            return false;
        }
        
        if (!m_graphsInfo || QNN_SUCCESS !=
                m_qnnInterface.graphRetrieve(m_contextVec[graphIdx], m_graphsInfo[graphIdx]->graphName,
                    &(m_graphsInfo[graphIdx]->graph))) {
            QNN_ERROR("Unable to retrieve graph handle for graph index = %zu", graphIdx);
            freeGraphsInfo(&m_graphsInfo, m_graphsCount);
            return false;
        }
    }

    if (true != freeContextConfigs(contextConfigs, contextConfigCount)) {
        QNN_ERROR("Couldn't free context configs");
        return false;
    }
    if (allContextConfigs) {
        free(allContextConfigs);
    }

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->afterCreateFromBinary()) {
            QNN_ERROR("Extensions Failure in afterCreateFromBinary()");
            return false;
        }
    }

    return true;
}

// Performance Setting for HTP
bool QnnApi::initializePerformance() {

    QnnDevice_Infrastructure_t deviceInfra = nullptr;
    if (QNN_SUCCESS != m_qnnInterface.deviceGetInfrastructure(&deviceInfra)) {
        QNN_ERROR("Failure in deviceGetInfrastructure()");
        return false;
    }

    QnnHtpDevice_Infrastructure_t *htpInfra = static_cast<QnnHtpDevice_Infrastructure_t *>(deviceInfra);
    m_perfInfra = htpInfra->perfInfra;
    uint32_t deviceId = 0;
    uint32_t coreId = 0;
    if (QNN_SUCCESS != m_perfInfra.createPowerConfigId(deviceId, coreId, &m_powerConfigId)) {
        QNN_ERROR("Failure in createPowerConfigId()");
        return false;
    }

    return true;
}

bool QnnApi::destroyPerformance() {
    if (QNN_SUCCESS != m_perfInfra.destroyPowerConfigId(m_powerConfigId)) {
        QNN_ERROR("Failure in destroyPowerConfigId()");
        return false;
    }

    return true;
}

bool QnnApi::boostPerformance() {
    // Initialize the power config and select the voltage corner values for the performance setting.
    QnnHtpPerfInfrastructure_PowerConfig_t powerConfig;
    memset(&powerConfig, 0, sizeof(powerConfig));

    powerConfig.option                               = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_DCVS_V3;
    powerConfig.dcvsV3Config.dcvsEnable              = 1;
    powerConfig.dcvsV3Config.setDcvsEnable           = 1;
    powerConfig.dcvsV3Config.contextId               = m_powerConfigId;

    // refer QnnHtpPerfInfrastructure.h
    powerConfig.dcvsV3Config.powerMode               = QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_PERFORMANCE_MODE;
    
    // Set Sleep-Disable latency parameter
    powerConfig.dcvsV3Config.setSleepDisable         = 0;
    powerConfig.dcvsV3Config.sleepDisable            = 0;

    // Set Sleep latency parameter
    powerConfig.dcvsV3Config.setSleepLatency         = 0;
    powerConfig.dcvsV3Config.sleepLatency            = 1000;        // range 40-2000 micro sec

    // Set Bus Clock Parameters (refer QnnHtpPerfInfrastructure.h)
    powerConfig.dcvsV3Config.setBusParams            = 1;
    powerConfig.dcvsV3Config.busVoltageCornerMin     = DCVS_VOLTAGE_VCORNER_TURBO_PLUS;
    powerConfig.dcvsV3Config.busVoltageCornerTarget  = DCVS_VOLTAGE_VCORNER_TURBO_PLUS;
    powerConfig.dcvsV3Config.busVoltageCornerMax     = DCVS_VOLTAGE_VCORNER_TURBO_PLUS;

    // set Core Clock Parameters (refer QnnHtpPerfInfrastructure.h)
    powerConfig.dcvsV3Config.setCoreParams           = 1;
    powerConfig.dcvsV3Config.coreVoltageCornerMin    = DCVS_VOLTAGE_VCORNER_TURBO_PLUS;
    powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_TURBO_PLUS;
    powerConfig.dcvsV3Config.coreVoltageCornerMax    = DCVS_VOLTAGE_VCORNER_TURBO_PLUS;

    // Set power config with different performance parameters
    const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&powerConfig, NULL};
    if (QNN_SUCCESS != m_perfInfra.setPowerConfig(m_powerConfigId, powerConfigs)) {
        QNN_ERROR("Failure in setPowerConfig() from boostPerformance");
        return false;
    }

    return true;
}

bool QnnApi::resetPerformance() {
    // Initialize the power config and select the voltage corner values for the performance setting.
    QnnHtpPerfInfrastructure_PowerConfig_t powerConfig;
    memset(&powerConfig, 0, sizeof(powerConfig));

    powerConfig.option                               = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_DCVS_V3;
    powerConfig.dcvsV3Config.dcvsEnable              = 1;
    powerConfig.dcvsV3Config.setDcvsEnable           = 1;
    powerConfig.dcvsV3Config.contextId               = m_powerConfigId;

    // refer QnnHtpPerfInfrastructure.h
    powerConfig.dcvsV3Config.powerMode               = QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_POWER_SAVER_MODE;
    
    // Set Sleep-Disable latency parameter
    powerConfig.dcvsV3Config.setSleepDisable         = 0;
    powerConfig.dcvsV3Config.sleepDisable            = 0;

    // Set Sleep latency parameter
    powerConfig.dcvsV3Config.setSleepLatency         = 0;
    powerConfig.dcvsV3Config.sleepLatency            = 1000;        // range 40-2000 micro sec

    // Set Bus Clock Parameters (refer QnnHtpPerfInfrastructure.h)
    powerConfig.dcvsV3Config.setBusParams            = 1;
    powerConfig.dcvsV3Config.busVoltageCornerMin     = DCVS_VOLTAGE_VCORNER_NOM;
    powerConfig.dcvsV3Config.busVoltageCornerTarget  = DCVS_VOLTAGE_VCORNER_NOM;
    powerConfig.dcvsV3Config.busVoltageCornerMax     = DCVS_VOLTAGE_VCORNER_TURBO;

    // set Core Clock Parameters (refer QnnHtpPerfInfrastructure.h)
    powerConfig.dcvsV3Config.setCoreParams           = 1;
    powerConfig.dcvsV3Config.coreVoltageCornerMin    = DCVS_VOLTAGE_VCORNER_NOM;
    powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_NOM;
    powerConfig.dcvsV3Config.coreVoltageCornerMax    = DCVS_VOLTAGE_VCORNER_TURBO;

    // Set power config with different performance parameters
    const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&powerConfig, NULL};
    if (QNN_SUCCESS != m_perfInfra.setPowerConfig(m_powerConfigId, powerConfigs)) {
        QNN_ERROR("Failure in setPowerConfig() from resetPerformance");
        return false;
    }

    return true;
}

bool QnnApi::initialize(std::string backendPath, std::vector<std::string> modelPathOrCachedBinaryPathVec,
                    BackendExtensionsConfigs backendExtensionsConfig,
                    PerfProfile parsedPerfProfile, 
                    ContextConfigs contextConfig,
                    std::vector<GraphConfigs> graphConfigs,
                    bool loadFromCachedBinary,
                    std::string systemLibraryPath,
                    bool debugModeRequested)
{
    if (modelPathOrCachedBinaryPathVec.size() > 1 &&
        false == loadFromCachedBinary) {
        QNN_ERROR("Currently only 1 model file is supported for this framework! \
            Although multiple context files are supported!");
        return false;
    }

    // Setting up Debug mode
    m_DebugModeRequested = debugModeRequested;
    if (m_DebugModeRequested) {
        QNN_WARN("Warning: Debug mode set to true.");
    }

    // Initialize the QNN run time
    if (false == getQnnInterface(backendPath))
    {
        QNN_ERROR("Qnn getQnnInterface FAILED!");
        return false;
    }

    if (loadFromCachedBinary)
    {
        if (false == getQnnSystemInterface(systemLibraryPath)) {
            QNN_ERROR("Qnn getQnnSystemInterface FAILED!");
            return false;
        }
    } else {
        if (false == loadModel(modelPathOrCachedBinaryPathVec[0])) {
            QNN_ERROR("Loading model FAILED!");
            return false;
        }
    }

    QnnLog_Level_t logLevel = QNN_LOG_LEVEL_ERROR;
    if (false == initializeLogging(logLevel)) {
        QNN_ERROR("Unable to Initialize logging in backend");
        return false;
    }

    // initialize backend extensions
    if (false == initializeBackendExtensions(backendExtensionsConfig, parsedPerfProfile)) {
        QNN_ERROR("Failure in initializing backend extensions.");
        return false;
    }

    if (false == initializeBackend())
    {
        QNN_ERROR("Qnn initializeBackend FAILED!");
        return false;
    }

    if (false == createDevice()) {
        QNN_ERROR("Device Creation failure");
        return false;
    }
    
    
    if (!loadFromCachedBinary)
    {
        if (false == createContext(contextConfig)) {
            QNN_ERROR("Qnn createContext FAILED!");
            return false;
        }

        if (false == composeGraphs(graphConfigs)) {
            QNN_ERROR("composeGraphs FAILED!");
            return false;
        }

        if (false == finalizeGraphs()) {
            QNN_ERROR("finalizeGraphs FAILED!");
            return false;
        }
    } else {
        if (false == createFromBinary(modelPathOrCachedBinaryPathVec, contextConfig)) {
            QNN_ERROR("Create From Binary FAILED!");
            return false;
        }
    }

    if (false == initializePerformance()) {
        QNN_ERROR("initialize Performance FAILED!");
        return false;
    }

    for (size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++) {
        m_graphNameToIndex[m_graphsInfo[graphIdx]->graphName] = graphIdx;
    }
    for (const auto& graphNameIndex : m_graphNameToIndex) {
        QNN_DEBUG("Found Graph name %s corresponding to index %d",
            graphNameIndex.first.c_str(), graphNameIndex.second);
    }

    return true;
}




bool QnnApi::initialize(std::string backendPath, std::string modelPathOrCachedBinaryPath,
                    BackendExtensionsConfigs backendExtensionsConfig,
                    PerfProfile parsedPerfProfile, 
                    ContextConfigs contextConfig,
                    std::vector<GraphConfigs> graphConfigs,
                    bool loadFromCachedBinary,
                    std::string systemLibraryPath,
                    bool debugModeRequested)
{

    return initialize(backendPath, std::vector<std::string>{modelPathOrCachedBinaryPath},
                        backendExtensionsConfig,
                        parsedPerfProfile, 
                        contextConfig,
                        graphConfigs,
                        loadFromCachedBinary,
                        systemLibraryPath,
                        debugModeRequested);
}

bool QnnApi::graphExecute(
    Qnn_Tensor_t* input, Qnn_Tensor_t* output, std::string graphName)
{
    QnnGraph_Config_t **customGraphConfigs{nullptr};
    uint32_t configCount{0};
    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
        if (!m_backendExtensions->interface1()->beforeExecute(
                        graphName.c_str(),
                        &customGraphConfigs, &configCount)) {
            QNN_ERROR("Extensions Failure in beforeExecute()");
            return false;
        }
        if (customGraphConfigs) {
#ifdef QNN_ENABLE_API_2x
            if (true != setGraphConfigsBeforeExecute(
                    m_graphsInfo[m_graphNameToIndex[graphName]]->graph, 
                    customGraphConfigs, configCount)) {
                QNN_ERROR("Failure in setGraphConfigsBeforeExecute()");
                return false;
            }
#endif
        }
    }

    if (true != boostPerformance()) {
        QNN_ERROR("Couldn't boost the performance");
        return false;
    }

    Qnn_ErrorHandle_t ret = QNN_GRAPH_NO_ERROR;
    try {
        ret = m_qnnInterface.graphExecute(m_graphsInfo[m_graphNameToIndex[graphName]]->graph,
                                          input,
                                          m_graphsInfo[m_graphNameToIndex[graphName]]->numInputTensors,
                                          output,
                                          m_graphsInfo[m_graphNameToIndex[graphName]]->numOutputTensors,
                                          nullptr,
                                          nullptr);
    } catch (const std::exception& ex) {
        QNN_ERROR("ERROR executing inference ret");
    } catch (...) {
        QNN_ERROR("ERROR executing inference ret");
    }

    if (true != resetPerformance()) {
        QNN_ERROR("Couldn't reset the performance");
        return false;
    }

    if (ret != QNN_GRAPH_NO_ERROR)
        return false;

    if (nullptr != m_backendExtensions && m_backendExtensions->interface1()) {
      if (!m_backendExtensions->interface1()->afterExecute()) {
          QNN_ERROR("Extensions Failure in afterExecute()");
          return false;
      }
    }

    return true;
}

bool QnnApi::getTensorQuantStatus(const Qnn_Tensor_t* tensor, double& scale, int32_t& offset)
{
    bool status = false;
    auto dataType = QNN_TENSOR_GET_DATA_TYPE(tensor);
    if (dataType == QNN_DATATYPE_UFIXED_POINT_8 ||
            dataType == QNN_DATATYPE_UFIXED_POINT_16) {
        status = true;
        scale  = QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale;
        offset = QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset;
    }

    return status;
}

bool QnnApi::getTensorNameAndShape(std::string& tensorName, std::vector<size_t>& tensorDims, TensorWrapper& tensorWrapper)
{
    Qnn_Tensor_t &tensor = GET_TENSOR_WRAPPER_TENSOR(tensorWrapper);
    tensorName = std::string(GET_TENSOR_WRAPPER_NAME(tensorWrapper));
    if (false == fillDims(tensorDims, 
                          QNN_TENSOR_GET_DIMENSIONS(tensor),
                          QNN_TENSOR_GET_RANK(tensor)))
        return false;

    tensorDims.push_back(g_qnnDataTypeToSize[QNN_TENSOR_GET_DATA_TYPE(tensor)]);
    return true;
}
