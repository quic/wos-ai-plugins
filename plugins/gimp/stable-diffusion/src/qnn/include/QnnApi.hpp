#pragma once

#include "BackendExtensions.hpp"
#include "QnnApiUtils.hpp"
#include "QnnInterface.h"
#include "QnnConfig.hpp"
#include "HTP/QnnHtpPerfInfrastructure.h"
#include "HTP/QnnHtpDevice.h"

#include <memory>

class QnnApi {
 private:
    const uint32_t s_graphConfigsReserveCount = 16;

    std::map<Qnn_DataType_t, size_t> g_qnnDataTypeToSize = {
        {QNN_DATATYPE_INT_8, 1},
        {QNN_DATATYPE_INT_16, 2},
        {QNN_DATATYPE_INT_32, 4},
        {QNN_DATATYPE_INT_64, 8},
        {QNN_DATATYPE_UINT_8, 1},
        {QNN_DATATYPE_UINT_16, 2},
        {QNN_DATATYPE_UINT_32, 4},
        {QNN_DATATYPE_UINT_64, 8},
        {QNN_DATATYPE_FLOAT_16, 2},
        {QNN_DATATYPE_FLOAT_32, 4},
        {QNN_DATATYPE_SFIXED_POINT_8, 1},
        {QNN_DATATYPE_SFIXED_POINT_16, 2},
        {QNN_DATATYPE_SFIXED_POINT_32, 4},
        {QNN_DATATYPE_UFIXED_POINT_8, 1},
        {QNN_DATATYPE_UFIXED_POINT_16, 2},
        {QNN_DATATYPE_UFIXED_POINT_32, 4},
        {QNN_DATATYPE_BOOL_8, 1},
    };

    // Model vars
#ifdef QNN_ENABLE_API_2x_P3
    typedef Qnn_ErrorHandle_t (*QnnInterfaceGetProvidersFn_t)(const QnnInterface_t*** providerList,
                                                              uint32_t* numProviders);
    typedef Qnn_ErrorHandle_t (*QnnSystemInterfaceGetProvidersFn_t)(const QnnSystemInterface_t*** providerList,
                                                                    uint32_t* numProviders);
#else
    typedef Qnn_ErrorHandle_t (*QnnInterfaceGetProvidersFn_t)(const QnnInterface_t** providerList,
                                                              uint32_t* numProviders);
    typedef Qnn_ErrorHandle_t (*QnnSystemInterfaceGetProvidersFn_t)(const QnnSystemInterface_t** providerList,
                                                                    uint32_t* numProviders);
#endif

    // Graph Related Function Handle Types
#ifdef QNN_ENABLE_API_2x_P2
    typedef ModelError_t (*ComposeGraphsFnHandleType_t)(
        Qnn_BackendHandle_t,
        QNN_INTERFACE_VER_TYPE,
        Qnn_ContextHandle_t,
        const GraphConfigInfo_t **,
        const uint32_t,
        GraphInfo_t ***,
        uint32_t *,
        bool,
        QnnLog_Callback_t,
        QnnLog_Level_t);
#else
    typedef ModelError_t (*ComposeGraphsFnHandleType_t)(
        void *,
        Qnn_ContextHandle_t,
        const GraphConfigInfo_t **,
        const uint32_t,
        GraphInfo_t ***,
        uint32_t *,
        bool);
#endif

    typedef ModelError_t (*FreeGraphInfoFnHandleType_t)(
        GraphInfo_t ***, uint32_t);

    void *m_libModelHandle{nullptr};
    void *m_backendHandle{nullptr};
    void *m_backendLibraryHandle{nullptr};

    QNN_INTERFACE_VER_TYPE m_qnnInterface{nullptr};
    QNN_SYSTEM_INTERFACE_VER_TYPE m_qnnSystemInterface{nullptr};
    std::unique_ptr<BackendExtensions> m_backendExtensions{nullptr};
    ComposeGraphsFnHandleType_t m_composeGraphsFnHandle{nullptr};
    FreeGraphInfoFnHandleType_t m_freeGraphInfoFnHandle{nullptr};
#ifdef QNN_ENABLE_API_2x_P2
    Qnn_LogHandle_t m_logHandle{nullptr};
    Qnn_DeviceHandle_t m_deviceHandle{nullptr};
#endif

    std::vector<Qnn_ContextHandle_t> m_contextVec;
    uint32_t m_graphsCount{0};
    GraphInfo_t** m_graphsInfo;
    std::unordered_map<std::string, uint32_t> m_graphNameToIndex;

    uint32_t m_backendConfigCount{0};
    QnnBackend_Config_t **m_backendConfigs{nullptr};

    QnnHtpDevice_PerfInfrastructure_t m_perfInfra;
    uint32_t m_powerConfigId = 1;

    bool m_isLogInitialized{false};
    bool m_isBackendInitialized{false};
    bool m_isContextCreated{false};

    // Variable to keep track of debug mode
    bool m_DebugModeRequested;

    bool getContextConfigs(QnnContext_Config_t ***configs,
                           uint32_t &contextConfigCount,
                           Qnn_Priority_t contextPriority);
    bool mergeAllContextConfigs(QnnContext_Config_t ***allCustomContextConfigs,
                                QnnContext_Config_t **customConfigs,
                                QnnContext_Config_t **contextConfigs,
                                uint32_t customConfigCount,
                                uint32_t contextConfigCount);
    bool freeContextConfigs(QnnContext_Config_t **contextConfigs,
                            uint32_t contextConfigCount);
    bool setGraphConfigsBeforeExecute(Qnn_GraphHandle_t graphHandle,
                                      QnnGraph_Config_t **graphConfigs,
                                      uint32_t configCount);

    bool getQnnInterface(std::string backendPath);
    bool getQnnSystemInterface(std::string systemLibraryPath);
    bool loadModel(std::string model_path);
    bool initializeLogging(const QnnLog_Level_t &logLevel);
    void terminateLog();
    bool initializeBackendExtensions(BackendExtensionsConfigs backendExtensionsConfig,
                                     PerfProfile parsedPerfProfile);
    bool initializeBackend();
    bool terminateBackend();
    bool createDevice();
    bool freeDevice();
    bool createContext(ContextConfigs contextConfig);
    bool freeContext();
    bool composeGraphs(std::vector<GraphConfigs> graphConfigs);
    bool finalizeGraphs();
    bool freeGraphs();
    bool createFromBinary(std::vector<std::string> cachedBinariesPathVec,
                          ContextConfigs contextConfig);
    bool initializePerformance();
    bool destroyPerformance();
    bool boostPerformance();
    bool resetPerformance();

 public:
    QnnApi(){};
    ~QnnApi();

    bool initialize(std::string backendPath, std::vector<std::string> modelPathOrCachedBinaryPathVec,
                    BackendExtensionsConfigs backendExtensionsConfig,
                    PerfProfile parsedPerfProfile=PerfProfile::BURST, 
                    ContextConfigs contextConfig=ContextConfigs(),
                    std::vector<GraphConfigs> graphConfigs={},
                    bool loadFromCachedBinary=false,
                    std::string systemLibraryPath="",
                    bool debugModeRequested=false);

    bool initialize(std::string backendPath, std::string modelPathOrCachedBinaryPath,
                    BackendExtensionsConfigs backendExtensionsConfig,
                    PerfProfile parsedPerfProfile=PerfProfile::BURST, 
                    ContextConfigs contextConfig=ContextConfigs(),
                    std::vector<GraphConfigs> graphConfigs={},
                    bool loadFromCachedBinary=false,
                    std::string systemLibraryPath="",
                    bool debugModeRequested=false);

    bool graphExecute(Qnn_Tensor_t* input, Qnn_Tensor_t* output, std::string graphName);

    QNN_INTERFACE_VER_TYPE* getQnnInterfaceVer() { return &m_qnnInterface; };
    GraphInfo_t**& getGraphsInfo() { return m_graphsInfo; };
    uint32_t getGraphsCount() { return m_graphsCount; };
    std::vector<Qnn_ContextHandle_t>& getContexts() { return m_contextVec; };

    bool getTensorQuantStatus(const Qnn_Tensor_t* tensor, double& scale, int32_t& offset);
    bool getTensorNameAndShape(std::string& tensorName, std::vector<size_t>& tensorDims, TensorWrapper& tensorWrapper);
};
