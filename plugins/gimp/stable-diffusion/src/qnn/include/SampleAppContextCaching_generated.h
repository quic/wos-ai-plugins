// ---------------------------------------------------------------------
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
// ---------------------------------------------------------------------


#ifndef FLATBUFFERS_GENERATED_SAMPLEAPPCONTEXTCACHING_H_
#define FLATBUFFERS_GENERATED_SAMPLEAPPCONTEXTCACHING_H_

#include "flatbuffers/flatbuffers.h"

struct QnnTensorInfo;
struct QnnTensorInfoT;

struct QnnGraphInfo;
struct QnnGraphInfoT;

struct ContextCache;
struct ContextCacheT;

struct QnnTensorInfoT : public flatbuffers::NativeTable {
  typedef QnnTensorInfo TableType;
  uint32_t id;
  std::string name;
  QnnTensorInfoT()
      : id(0) {
  }
};

struct QnnTensorInfo FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef QnnTensorInfoT NativeTableType;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ID = 4,
    VT_NAME = 6
  };
  uint32_t id() const {
    return GetField<uint32_t>(VT_ID, 0);
  }
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ID) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           verifier.EndTable();
  }
  QnnTensorInfoT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(QnnTensorInfoT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<QnnTensorInfo> Pack(flatbuffers::FlatBufferBuilder &_fbb, const QnnTensorInfoT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct QnnTensorInfoBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(uint32_t id) {
    fbb_.AddElement<uint32_t>(QnnTensorInfo::VT_ID, id, 0);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(QnnTensorInfo::VT_NAME, name);
  }
  explicit QnnTensorInfoBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  QnnTensorInfoBuilder &operator=(const QnnTensorInfoBuilder &);
  flatbuffers::Offset<QnnTensorInfo> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<QnnTensorInfo>(end);
    return o;
  }
};

inline flatbuffers::Offset<QnnTensorInfo> CreateQnnTensorInfo(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0) {
  QnnTensorInfoBuilder builder_(_fbb);
  builder_.add_name(name);
  builder_.add_id(id);
  return builder_.Finish();
}

inline flatbuffers::Offset<QnnTensorInfo> CreateQnnTensorInfoDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    const char *name = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return CreateQnnTensorInfo(
      _fbb,
      id,
      name__);
}

flatbuffers::Offset<QnnTensorInfo> CreateQnnTensorInfo(flatbuffers::FlatBufferBuilder &_fbb, const QnnTensorInfoT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct QnnGraphInfoT : public flatbuffers::NativeTable {
  typedef QnnGraphInfo TableType;
  std::string name;
  uint32_t inputTensorsCount;
  std::vector<std::unique_ptr<QnnTensorInfoT>> inputTensorsInfo;
  uint32_t outputTensorsCount;
  std::vector<std::unique_ptr<QnnTensorInfoT>> outputTensorsInfo;
  QnnGraphInfoT()
      : inputTensorsCount(0),
        outputTensorsCount(0) {
  }
};

struct QnnGraphInfo FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef QnnGraphInfoT NativeTableType;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_INPUTTENSORSCOUNT = 6,
    VT_INPUTTENSORSINFO = 8,
    VT_OUTPUTTENSORSCOUNT = 10,
    VT_OUTPUTTENSORSINFO = 12
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  uint32_t inputTensorsCount() const {
    return GetField<uint32_t>(VT_INPUTTENSORSCOUNT, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>> *inputTensorsInfo() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>> *>(VT_INPUTTENSORSINFO);
  }
  uint32_t outputTensorsCount() const {
    return GetField<uint32_t>(VT_OUTPUTTENSORSCOUNT, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>> *outputTensorsInfo() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>> *>(VT_OUTPUTTENSORSINFO);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint32_t>(verifier, VT_INPUTTENSORSCOUNT) &&
           VerifyOffset(verifier, VT_INPUTTENSORSINFO) &&
           verifier.VerifyVector(inputTensorsInfo()) &&
           verifier.VerifyVectorOfTables(inputTensorsInfo()) &&
           VerifyField<uint32_t>(verifier, VT_OUTPUTTENSORSCOUNT) &&
           VerifyOffset(verifier, VT_OUTPUTTENSORSINFO) &&
           verifier.VerifyVector(outputTensorsInfo()) &&
           verifier.VerifyVectorOfTables(outputTensorsInfo()) &&
           verifier.EndTable();
  }
  QnnGraphInfoT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(QnnGraphInfoT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<QnnGraphInfo> Pack(flatbuffers::FlatBufferBuilder &_fbb, const QnnGraphInfoT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct QnnGraphInfoBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(QnnGraphInfo::VT_NAME, name);
  }
  void add_inputTensorsCount(uint32_t inputTensorsCount) {
    fbb_.AddElement<uint32_t>(QnnGraphInfo::VT_INPUTTENSORSCOUNT, inputTensorsCount, 0);
  }
  void add_inputTensorsInfo(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>>> inputTensorsInfo) {
    fbb_.AddOffset(QnnGraphInfo::VT_INPUTTENSORSINFO, inputTensorsInfo);
  }
  void add_outputTensorsCount(uint32_t outputTensorsCount) {
    fbb_.AddElement<uint32_t>(QnnGraphInfo::VT_OUTPUTTENSORSCOUNT, outputTensorsCount, 0);
  }
  void add_outputTensorsInfo(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>>> outputTensorsInfo) {
    fbb_.AddOffset(QnnGraphInfo::VT_OUTPUTTENSORSINFO, outputTensorsInfo);
  }
  explicit QnnGraphInfoBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  QnnGraphInfoBuilder &operator=(const QnnGraphInfoBuilder &);
  flatbuffers::Offset<QnnGraphInfo> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<QnnGraphInfo>(end);
    return o;
  }
};

inline flatbuffers::Offset<QnnGraphInfo> CreateQnnGraphInfo(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    uint32_t inputTensorsCount = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>>> inputTensorsInfo = 0,
    uint32_t outputTensorsCount = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<QnnTensorInfo>>> outputTensorsInfo = 0) {
  QnnGraphInfoBuilder builder_(_fbb);
  builder_.add_outputTensorsInfo(outputTensorsInfo);
  builder_.add_outputTensorsCount(outputTensorsCount);
  builder_.add_inputTensorsInfo(inputTensorsInfo);
  builder_.add_inputTensorsCount(inputTensorsCount);
  builder_.add_name(name);
  return builder_.Finish();
}

inline flatbuffers::Offset<QnnGraphInfo> CreateQnnGraphInfoDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    uint32_t inputTensorsCount = 0,
    const std::vector<flatbuffers::Offset<QnnTensorInfo>> *inputTensorsInfo = nullptr,
    uint32_t outputTensorsCount = 0,
    const std::vector<flatbuffers::Offset<QnnTensorInfo>> *outputTensorsInfo = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto inputTensorsInfo__ = inputTensorsInfo ? _fbb.CreateVector<flatbuffers::Offset<QnnTensorInfo>>(*inputTensorsInfo) : 0;
  auto outputTensorsInfo__ = outputTensorsInfo ? _fbb.CreateVector<flatbuffers::Offset<QnnTensorInfo>>(*outputTensorsInfo) : 0;
  return CreateQnnGraphInfo(
      _fbb,
      name__,
      inputTensorsCount,
      inputTensorsInfo__,
      outputTensorsCount,
      outputTensorsInfo__);
}

flatbuffers::Offset<QnnGraphInfo> CreateQnnGraphInfo(flatbuffers::FlatBufferBuilder &_fbb, const QnnGraphInfoT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct ContextCacheT : public flatbuffers::NativeTable {
  typedef ContextCache TableType;
  uint32_t graphsCount;
  std::vector<std::unique_ptr<QnnGraphInfoT>> graphsInfo;
  uint32_t binaryCacheSize;
  std::vector<uint8_t> binaryCache;
  ContextCacheT()
      : graphsCount(0),
        binaryCacheSize(0) {
  }
};

struct ContextCache FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ContextCacheT NativeTableType;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_GRAPHSCOUNT = 4,
    VT_GRAPHSINFO = 6,
    VT_BINARYCACHESIZE = 8,
    VT_BINARYCACHE = 10
  };
  uint32_t graphsCount() const {
    return GetField<uint32_t>(VT_GRAPHSCOUNT, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<QnnGraphInfo>> *graphsInfo() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<QnnGraphInfo>> *>(VT_GRAPHSINFO);
  }
  uint32_t binaryCacheSize() const {
    return GetField<uint32_t>(VT_BINARYCACHESIZE, 0);
  }
  const flatbuffers::Vector<uint8_t> *binaryCache() const {
    return GetPointer<const flatbuffers::Vector<uint8_t> *>(VT_BINARYCACHE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_GRAPHSCOUNT) &&
           VerifyOffset(verifier, VT_GRAPHSINFO) &&
           verifier.VerifyVector(graphsInfo()) &&
           verifier.VerifyVectorOfTables(graphsInfo()) &&
           VerifyField<uint32_t>(verifier, VT_BINARYCACHESIZE) &&
           VerifyOffset(verifier, VT_BINARYCACHE) &&
           verifier.VerifyVector(binaryCache()) &&
           verifier.EndTable();
  }
  ContextCacheT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(ContextCacheT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<ContextCache> Pack(flatbuffers::FlatBufferBuilder &_fbb, const ContextCacheT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct ContextCacheBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_graphsCount(uint32_t graphsCount) {
    fbb_.AddElement<uint32_t>(ContextCache::VT_GRAPHSCOUNT, graphsCount, 0);
  }
  void add_graphsInfo(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<QnnGraphInfo>>> graphsInfo) {
    fbb_.AddOffset(ContextCache::VT_GRAPHSINFO, graphsInfo);
  }
  void add_binaryCacheSize(uint32_t binaryCacheSize) {
    fbb_.AddElement<uint32_t>(ContextCache::VT_BINARYCACHESIZE, binaryCacheSize, 0);
  }
  void add_binaryCache(flatbuffers::Offset<flatbuffers::Vector<uint8_t>> binaryCache) {
    fbb_.AddOffset(ContextCache::VT_BINARYCACHE, binaryCache);
  }
  explicit ContextCacheBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ContextCacheBuilder &operator=(const ContextCacheBuilder &);
  flatbuffers::Offset<ContextCache> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ContextCache>(end);
    return o;
  }
};

inline flatbuffers::Offset<ContextCache> CreateContextCache(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t graphsCount = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<QnnGraphInfo>>> graphsInfo = 0,
    uint32_t binaryCacheSize = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> binaryCache = 0) {
  ContextCacheBuilder builder_(_fbb);
  builder_.add_binaryCache(binaryCache);
  builder_.add_binaryCacheSize(binaryCacheSize);
  builder_.add_graphsInfo(graphsInfo);
  builder_.add_graphsCount(graphsCount);
  return builder_.Finish();
}

inline flatbuffers::Offset<ContextCache> CreateContextCacheDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t graphsCount = 0,
    const std::vector<flatbuffers::Offset<QnnGraphInfo>> *graphsInfo = nullptr,
    uint32_t binaryCacheSize = 0,
    const std::vector<uint8_t> *binaryCache = nullptr) {
  auto graphsInfo__ = graphsInfo ? _fbb.CreateVector<flatbuffers::Offset<QnnGraphInfo>>(*graphsInfo) : 0;
  auto binaryCache__ = binaryCache ? _fbb.CreateVector<uint8_t>(*binaryCache) : 0;
  return CreateContextCache(
      _fbb,
      graphsCount,
      graphsInfo__,
      binaryCacheSize,
      binaryCache__);
}

flatbuffers::Offset<ContextCache> CreateContextCache(flatbuffers::FlatBufferBuilder &_fbb, const ContextCacheT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

inline QnnTensorInfoT *QnnTensorInfo::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  auto _o = new QnnTensorInfoT();
  UnPackTo(_o, _resolver);
  return _o;
}

inline void QnnTensorInfo::UnPackTo(QnnTensorInfoT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = id(); _o->id = _e; };
  { auto _e = name(); if (_e) _o->name = _e->str(); };
}

inline flatbuffers::Offset<QnnTensorInfo> QnnTensorInfo::Pack(flatbuffers::FlatBufferBuilder &_fbb, const QnnTensorInfoT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return CreateQnnTensorInfo(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<QnnTensorInfo> CreateQnnTensorInfo(flatbuffers::FlatBufferBuilder &_fbb, const QnnTensorInfoT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const QnnTensorInfoT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _id = _o->id;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  return CreateQnnTensorInfo(
      _fbb,
      _id,
      _name);
}

inline QnnGraphInfoT *QnnGraphInfo::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  auto _o = new QnnGraphInfoT();
  UnPackTo(_o, _resolver);
  return _o;
}

inline void QnnGraphInfo::UnPackTo(QnnGraphInfoT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = name(); if (_e) _o->name = _e->str(); };
  { auto _e = inputTensorsCount(); _o->inputTensorsCount = _e; };
  { auto _e = inputTensorsInfo(); if (_e) { _o->inputTensorsInfo.resize(_e->size()); for (flatbuffers::uoffset_t _i = 0; _i < _e->size(); _i++) { _o->inputTensorsInfo[_i] = std::unique_ptr<QnnTensorInfoT>(_e->Get(_i)->UnPack(_resolver)); } } };
  { auto _e = outputTensorsCount(); _o->outputTensorsCount = _e; };
  { auto _e = outputTensorsInfo(); if (_e) { _o->outputTensorsInfo.resize(_e->size()); for (flatbuffers::uoffset_t _i = 0; _i < _e->size(); _i++) { _o->outputTensorsInfo[_i] = std::unique_ptr<QnnTensorInfoT>(_e->Get(_i)->UnPack(_resolver)); } } };
}

inline flatbuffers::Offset<QnnGraphInfo> QnnGraphInfo::Pack(flatbuffers::FlatBufferBuilder &_fbb, const QnnGraphInfoT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return CreateQnnGraphInfo(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<QnnGraphInfo> CreateQnnGraphInfo(flatbuffers::FlatBufferBuilder &_fbb, const QnnGraphInfoT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const QnnGraphInfoT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _name = _o->name.empty() ? 0 : _fbb.CreateString(_o->name);
  auto _inputTensorsCount = _o->inputTensorsCount;
  auto _inputTensorsInfo = _o->inputTensorsInfo.size() ? _fbb.CreateVector<flatbuffers::Offset<QnnTensorInfo>> (_o->inputTensorsInfo.size(), [](size_t i, _VectorArgs *__va) { return CreateQnnTensorInfo(*__va->__fbb, __va->__o->inputTensorsInfo[i].get(), __va->__rehasher); }, &_va ) : 0;
  auto _outputTensorsCount = _o->outputTensorsCount;
  auto _outputTensorsInfo = _o->outputTensorsInfo.size() ? _fbb.CreateVector<flatbuffers::Offset<QnnTensorInfo>> (_o->outputTensorsInfo.size(), [](size_t i, _VectorArgs *__va) { return CreateQnnTensorInfo(*__va->__fbb, __va->__o->outputTensorsInfo[i].get(), __va->__rehasher); }, &_va ) : 0;
  return CreateQnnGraphInfo(
      _fbb,
      _name,
      _inputTensorsCount,
      _inputTensorsInfo,
      _outputTensorsCount,
      _outputTensorsInfo);
}

inline ContextCacheT *ContextCache::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  auto _o = new ContextCacheT();
  UnPackTo(_o, _resolver);
  return _o;
}

inline void ContextCache::UnPackTo(ContextCacheT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = graphsCount(); _o->graphsCount = _e; };
  { auto _e = graphsInfo(); if (_e) { _o->graphsInfo.resize(_e->size()); for (flatbuffers::uoffset_t _i = 0; _i < _e->size(); _i++) { _o->graphsInfo[_i] = std::unique_ptr<QnnGraphInfoT>(_e->Get(_i)->UnPack(_resolver)); } } };
  { auto _e = binaryCacheSize(); _o->binaryCacheSize = _e; };
  { auto _e = binaryCache(); if (_e) { _o->binaryCache.resize(_e->size()); for (flatbuffers::uoffset_t _i = 0; _i < _e->size(); _i++) { _o->binaryCache[_i] = _e->Get(_i); } } };
}

inline flatbuffers::Offset<ContextCache> ContextCache::Pack(flatbuffers::FlatBufferBuilder &_fbb, const ContextCacheT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return CreateContextCache(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<ContextCache> CreateContextCache(flatbuffers::FlatBufferBuilder &_fbb, const ContextCacheT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const ContextCacheT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _graphsCount = _o->graphsCount;
  auto _graphsInfo = _o->graphsInfo.size() ? _fbb.CreateVector<flatbuffers::Offset<QnnGraphInfo>> (_o->graphsInfo.size(), [](size_t i, _VectorArgs *__va) { return CreateQnnGraphInfo(*__va->__fbb, __va->__o->graphsInfo[i].get(), __va->__rehasher); }, &_va ) : 0;
  auto _binaryCacheSize = _o->binaryCacheSize;
  auto _binaryCache = _o->binaryCache.size() ? _fbb.CreateVector(_o->binaryCache) : 0;
  return CreateContextCache(
      _fbb,
      _graphsCount,
      _graphsInfo,
      _binaryCacheSize,
      _binaryCache);
}

inline const ContextCache *GetContextCache(const void *buf) {
  return flatbuffers::GetRoot<ContextCache>(buf);
}

inline const ContextCache *GetSizePrefixedContextCache(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<ContextCache>(buf);
}

inline bool VerifyContextCacheBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<ContextCache>(nullptr);
}

inline bool VerifySizePrefixedContextCacheBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<ContextCache>(nullptr);
}

inline void FinishContextCacheBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<ContextCache> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedContextCacheBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<ContextCache> root) {
  fbb.FinishSizePrefixed(root);
}

inline std::unique_ptr<ContextCacheT> UnPackContextCache(
    const void *buf,
    const flatbuffers::resolver_function_t *res = nullptr) {
  return std::unique_ptr<ContextCacheT>(GetContextCache(buf)->UnPack(res));
}

#endif  // FLATBUFFERS_GENERATED_SAMPLEAPPCONTEXTCACHING_H_
