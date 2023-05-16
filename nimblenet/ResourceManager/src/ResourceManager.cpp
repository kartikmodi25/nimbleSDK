/* This is an resource utility script from where one can
load model, model plans, save or cache and perform inference from device */

#include "ResourceManager.h"

#include <fstream>

#include "NativeInterface.h"
#include "ServerAPI.h"
#include "client.h"
#include "util.h"

using namespace std;
using json = nlohmann::json;
#define PARTSIZE 1000000

ResourceManager::ResourceManager(CommandCenter* commandCenter) { _commandCenter = commandCenter; }

bool ResourceManager::load_plan_from_device(const string& modelId) {
  PlanData planData = get_inference_plan_data_from_device(modelId);
  if (planData.valid) {
    if (!_commandCenter->get_mlExecutor()->load_plan(modelId, planData)) {
      LOG_TO_ERROR("Plan downloaded for modelId=%s, could not be loaded by Executor",
                   modelId.c_str());
      return false;
    }
  } else {
    return false;
  }
  LOG_TO_INFO("Loaded modelId=%s version=%s from device", modelId.c_str(),
              planData.version.c_str());
  return true;
}

int ResourceManager::load_plan(const string& modelId) {
  bool planExpired = false;
  std::string deviceId = _commandCenter->get_config().deviceId;
  std::string clientId = _commandCenter->get_config().clientId;
  VersionCheckResponse inferenceVersionCheckResponse = ServerAPI::version_check(
      VersionRequest(BaseAPIRequest(deviceId, clientId), "inference", modelId));
  if (inferenceVersionCheckResponse.base.status == SUCCESS) {
    if (_commandCenter->get_mlExecutor()->get_plan_version(modelId) ==
        inferenceVersionCheckResponse.version) {
      _commandCenter->get_mlExecutor()->plan_updated(modelId, true);
      return SUCCESS;
    } else {
      string plan = "";
      plan.reserve(PARTSIZE * inferenceVersionCheckResponse.fileParts);
      auto start = std::chrono::high_resolution_clock::now();
      for (int part = 1; part <= inferenceVersionCheckResponse.fileParts; part++) {
        PlanRequest planReq(BaseAPIRequest(deviceId, clientId), part, modelId,
                            inferenceVersionCheckResponse.version);
        const auto response = ServerAPI::get_plan(planReq);
        if (response->r.statusCode != SUCCESS) {
          LOG_TO_ERROR("GetPlan for modelId=%s, version=%s, returned with status=%d.",
                       modelId.c_str(), inferenceVersionCheckResponse.version.c_str(),
                       response->r.statusCode);

          _commandCenter->get_mlExecutor()->plan_updated(modelId, false);
          return response->r.statusCode;
        }
        plan += string(response->r.body, response->r.bodyLength);
      }
      auto stop = std::chrono::high_resolution_clock::now();
      int duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
      nlohmann::json downloadMetric;
      downloadMetric["downloadSpeed"] = ((float)plan.length() * 1000000) / duration;
      downloadMetric["modelSize"] = plan.length();
      downloadMetric["modelName"] = modelId;
      downloadMetric["modelVersion"] = inferenceVersionCheckResponse.version;
      _commandCenter->log_metrics(MODELDOWNLOADMETRIC, downloadMetric);
      PlanData planData;
      planData.version = inferenceVersionCheckResponse.version;
      planData.extraFields = inferenceVersionCheckResponse.extras;

      planData.planLength = plan.length();
      planData.plan = plan;
      save_inference_plan_data_on_device(modelId, planData);
      if (!_commandCenter->get_mlExecutor()->load_plan(modelId, planData)) {
        LOG_TO_FATAL("Plan downloaded for modelId=%s, could not be loaded by Executor",
                     modelId.c_str());
        _commandCenter->get_mlExecutor()->plan_updated(modelId, false);
        return EXECUTOR_LOAD_MODEL_ERR;
      }
    }

  } else {
    LOG_TO_ERROR("Get Version failed with status=%d, for modelId=%s error=%s",
                 inferenceVersionCheckResponse.base.status, modelId.c_str(),
                 inferenceVersionCheckResponse.base.error.description.c_str());
    return inferenceVersionCheckResponse.base.status;
  }
  LOG_TO_INFO("Downloaded modelId=%s version=%s successfully", modelId.c_str(),
              inferenceVersionCheckResponse.version.c_str());
  _commandCenter->get_mlExecutor()->plan_updated(modelId, true);
  return SUCCESS;
}

void ResourceManager::update_plan_if_required(const std::string& modelId) {
  bool retriesLeft = _commandCenter->get_mlExecutor()->can_model_retry(modelId);
  if (retriesLeft) {
    int ret = load_plan(modelId);
    if (ret == SUCCESS) {  // setting retries 0 for success as well as non retriable errors
      _commandCenter->get_mlExecutor()->update_model_retries(modelId);
    } else if (ret > 0) {  // All non retriable errors
      _commandCenter->get_mlExecutor()->update_model_retries(modelId);
    } else {  // For retrieable errors
      _commandCenter->get_mlExecutor()->update_model_retries(modelId);
      // no need to send callback to android/ios for retriable failure
    }
  } else {
    // LOG_TO_DEBUG("No retries left for %s",modelId.c_str());
  }
}

bool ResourceManager::check_model_loaded_for_inference(const string& modelId) {
  return _commandCenter->get_mlExecutor()->check_model_loaded(modelId);
}

PlanData ResourceManager::get_inference_plan_data_from_device(const string& modelId) {
  PlanData planData;
  string planMetadatastring;
  if (!NativeInterface::get_file_from_device(modelId + INFERENCEMETADATAFILENAME,
                                             planMetadatastring)) {
    LOG_TO_ERROR("Could not load plan metadata for model %s from device", modelId.c_str());
    return planData;
  }
  planData = JSONParser::get<PlanData>(planMetadatastring);
  if (!NativeInterface::get_file_from_device(modelId + INFERENCEFILENAME, planData.plan)) {
    LOG_TO_ERROR("Could not load plan file for model %s from device", modelId.c_str());
    return planData;
  }
  if (planData.planLength != planData.plan.length()) {
    planData.valid = false;
    LOG_TO_ERROR("Plan stored in device is not the same length as plan Metadata for Model %s",
                 modelId.c_str());
    return planData;
  }
  planData.valid = true;
  return planData;
}

bool ResourceManager::save_inference_plan_data_on_device(const string& modelId,
                                                         const PlanData& planData) {
  return NativeInterface::save_file_on_device(nlohmann::json(planData).dump(),
                                              modelId + INFERENCEMETADATAFILENAME) &&
         NativeInterface::save_file_on_device(planData.plan, modelId + INFERENCEFILENAME);
}

FeatureData ResourceManager::get_feature_data_from_device(const string& featureName) {
  std::string featureDataString;
  std::string featureVectorBytes;
  if (!NativeInterface::get_file_from_device(featureName + FEATURESTOREMETADATAFILENAME,
                                             featureDataString))
    return FeatureData();
  FeatureData featureData = JSONParser::get<FeatureData>(featureDataString);
  if (!featureData.valid) return FeatureData();
  uint64_t seconds = time(NULL);
  if (featureData.expiry < seconds) {
    LOG_TO_DEBUG("Feature=%s expired on device", featureName.c_str());
    return FeatureData();
  }
  if (!NativeInterface::get_file_from_device(featureName + FEATURESTOREFILENAME,
                                             featureVectorBytes))
    return FeatureData();
  featureData.feature = new char[featureVectorBytes.size()];
  memcpy(featureData.feature, &featureVectorBytes[0], featureVectorBytes.size());
  featureData.featureLength = featureVectorBytes.size();
  return std::move(featureData);
}

bool ResourceManager::save_feature_data_on_device(const string& featureName,
                                                  const FeatureData& featureData) {
  // saving in two parts as we cannot save feature vector in a json as it has unreadable bytes
  if (!NativeInterface::save_file_on_device(string(featureData.feature, featureData.featureLength),
                                            featureName + FEATURESTOREFILENAME))
    return false;
  return NativeInterface::save_file_on_device(nlohmann::json(featureData).dump(),
                                              featureName + FEATURESTOREMETADATAFILENAME);
}

bool ResourceManager::load_feature_from_device(const string& featureName) {
  FeatureData featureData = get_feature_data_from_device(featureName);
  if (featureData.valid) {
    if (!_commandCenter->get_feature_store_manager()->load_feature_store(featureName,
                                                                         featureData)) {
      LOG_TO_ERROR(
          "Feature loaded for featureName=%s from device, could not be loaded by "
          "featureStoreManager",
          featureName.c_str());
      delete[] featureData.feature;
      return false;
    }
    delete[] featureData.feature;
  } else {
    return false;
  }
  LOG_TO_INFO("Loaded featureName=%s from device", featureName.c_str());
  return true;
}

void ResourceManager::update_feature_if_required(const std::string& featureName) {
  // Add logic for looping for syncing featureStore
  if (_commandCenter->get_feature_store_manager()->should_download(featureName)) {
    FeatureVersionCheckResponse versionResponse = ServerAPI::get_feature_version(featureName);
    if (versionResponse.base.status == SUCCESS) {
      string previousVersion =
          _commandCenter->get_feature_store_manager()->get_version(featureName);
      if (previousVersion != versionResponse.version) {
        const auto featureResponse = ServerAPI::get_feature(featureName, versionResponse.version);
        if (featureResponse->r.statusCode != SUCCESS) {
          LOG_TO_ERROR("Feature=%s could not be downloaded. statusCode=%d", featureName.c_str(),
                       featureResponse->r.statusCode);
          return;
        }
        FeatureData featureData;
        featureData.feature = featureResponse->r.body;
        featureData.featureLength = featureResponse->r.bodyLength;
        featureData.dataType = versionResponse.dataType;
        featureData.version = versionResponse.version;
        featureData.expiry = versionResponse.expiry;
        featureData.valid = true;
        if (!_commandCenter->get_feature_store_manager()->load_feature_store(featureName,
                                                                             featureData)) {
          LOG_TO_ERROR("Feature=%s could not be loaded even after successful download",
                       featureName.c_str());
          return;
        }
        save_feature_data_on_device(featureName, featureData);
      }
      _commandCenter->get_feature_store_manager()->set_upto_date(featureName);
    } else {
      LOG_TO_ERROR("Get Version failed with status=%d, for feature=%s error=%s",
                   versionResponse.base.status, featureName.c_str(),
                   versionResponse.base.error.description.c_str());
      return;
    }
    LOG_TO_INFO("Loaded featureName=%s, version=%s from cloud", featureName.c_str(),
                versionResponse.version.c_str());
  }
}
