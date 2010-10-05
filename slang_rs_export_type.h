#ifndef _SLANG_COMPILER_RS_EXPORT_TYPE_H
#define _SLANG_COMPILER_RS_EXPORT_TYPE_H

#include <set>
#include <list>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"

#define GET_CANONICAL_TYPE(T) \
  (((T) == NULL) ? NULL : (T)->getCanonicalTypeInternal().getTypePtr())
#define UNSAFE_CAST_TYPE(TT, T) \
  static_cast<TT*>(T->getCanonicalTypeInternal().getTypePtr())
#define GET_CONSTANT_ARRAY_ELEMENT_TYPE(T) \
  (((T) == NULL) ? NULL : \
                   GET_CANONICAL_TYPE((T)->getElementType().getTypePtr()))
#define GET_EXT_VECTOR_ELEMENT_TYPE(T) \
  (((T) == NULL) ? NULL : \
                   GET_CANONICAL_TYPE((T)->getElementType().getTypePtr()))
#define GET_POINTEE_TYPE(T) \
  (((T) == NULL) ? NULL : \
                   GET_CANONICAL_TYPE((T)->getPointeeType().getTypePtr()))

namespace llvm {
  class Type;
}   // namespace llvm

namespace slang {

  class RSContext;
  class RSExportPrimitiveType;
  class RSExportConstantArrayType;
  class RSExportVectorType;
  class RSExportRecordType;
  class RSExportFunction;

class RSExportType {
  friend class RSExportElement;
 public:
  typedef enum {
    ExportClassPrimitive,
    ExportClassPointer,
    ExportClassConstantArray,
    ExportClassVector,
    ExportClassRecord
  } ExportClass;

 private:
  RSContext *mContext;
  std::string mName;

  // Cache the result after calling convertToLLVMType() at the first time
  mutable const llvm::Type *mLLVMType;

 protected:
  RSExportType(RSContext *Context, const llvm::StringRef &Name);

  // Let's make it private since there're some prerequisites to call this
  // function.
  //
  // @T was normalized by calling RSExportType::TypeExportable().
  // @TypeName was retrieve from RSExportType::GetTypeName() before calling
  //           this.
  //
  static RSExportType *Create(RSContext *Context,
                              const clang::Type *T,
                              const llvm::StringRef &TypeName);

  static llvm::StringRef GetTypeName(const clang::Type *T);
  // Return the type that can be used to create RSExportType, will always return
  // the canonical type
  static const clang::Type
  *TypeExportable(const clang::Type *T,
                  // Contain the checked type for recursion
                  llvm::SmallPtrSet<const clang::Type*, 8> &SPS);

  // This function convert the RSExportType to LLVM type. Actually, it should be
  // "convert Clang type to LLVM type." However, clang doesn't make this API
  // (lib/CodeGen/CodeGenTypes.h) public, we need to do by ourselves.
  //
  // Once we can get LLVM type, we can use LLVM to get alignment information,
  // allocation size of a given type and structure layout that LLVM used
  // (all of these information are target dependent) without dealing with these
  // by ourselves.
  virtual const llvm::Type *convertToLLVMType() const = 0;

  virtual ~RSExportType() {}
 public:
  static bool NormalizeType(const clang::Type *&T, llvm::StringRef &TypeName);
  // @T may not be normalized
  static RSExportType *Create(RSContext *Context, const clang::Type *T);
  static RSExportType *CreateFromDecl(RSContext *Context,
                                      const clang::VarDecl *VD);

  static const clang::Type *GetTypeOfDecl(const clang::DeclaratorDecl *DD);

  virtual ExportClass getClass() const = 0;

  inline const llvm::Type *getLLVMType() const {
    if (mLLVMType == NULL)
      mLLVMType = convertToLLVMType();
    return mLLVMType;
  }

  // Return the number of bits necessary to hold the specified RSExportType
  static size_t GetTypeStoreSize(const RSExportType *ET);

  // The size of allocation of specified RSExportType (alignment considered)
  static size_t GetTypeAllocSize(const RSExportType *ET);
  static unsigned char GetTypeAlignment(const RSExportType *ET);

  const std::string &getName() const { return mName; }
  inline RSContext *getRSContext() const { return mContext; }
};  // RSExportType

// Primitive types
class RSExportPrimitiveType : public RSExportType {
  friend class RSExportType;
  friend class RSExportElement;
 public:
  // From graphics/java/android/renderscript/Element.java: Element.DataType
  typedef enum {
    DataTypeUnknown = -1,

    DataTypeFloat16 = 0,
    DataTypeFloat32,
    DataTypeFloat64,
    DataTypeSigned8,
    DataTypeSigned16,
    DataTypeSigned32,
    DataTypeSigned64,
    DataTypeUnsigned8,
    DataTypeUnsigned16,
    DataTypeUnsigned32,
    DataTypeUnSigned64,

    DataTypeBoolean,

    DataTypeUnsigned565,
    DataTypeUnsigned5551,
    DataTypeUnsigned4444,

    DataTypeRSMatrix2x2,
    DataTypeRSMatrix3x3,
    DataTypeRSMatrix4x4,

    DataTypeRSElement,
    DataTypeRSType,
    DataTypeRSAllocation,
    DataTypeRSSampler,
    DataTypeRSScript,
    DataTypeRSMesh,
    DataTypeRSProgramFragment,
    DataTypeRSProgramVertex,
    DataTypeRSProgramRaster,
    DataTypeRSProgramStore,
    DataTypeRSFont,

    DataTypeMax
  } DataType;

  // From graphics/java/android/renderscript/Element.java: Element.DataKind
  typedef enum {
    DataKindUser,
    DataKindPixelL,
    DataKindPixelA,
    DataKindPixelLA,
    DataKindPixelRGB,
    DataKindPixelRGBA
  } DataKind;

 private:
  DataType mType;
  DataKind mKind;
  bool mNormalized;

  typedef llvm::StringMap<DataType> RSObjectTypeMapTy;
  static RSObjectTypeMapTy *RSObjectTypeMap;

  static llvm::Type *RSObjectLLVMType;

  static const size_t SizeOfDataTypeInBits[];
  // @T was normalized by calling RSExportType::TypeExportable() before calling
  // this.
  // @TypeName was retrieved from RSExportType::GetTypeName() before calling
  // this
  static RSExportPrimitiveType *Create(RSContext *Context,
                                       const clang::Type *T,
                                       const llvm::StringRef &TypeName,
                                       DataKind DK = DataKindUser,
                                       bool Normalized = false);

 protected:
  // T is normalized by calling RSExportType::TypeExportable() before
  // calling this
  static bool IsPrimitiveType(const clang::Type *T);

  static DataType GetDataType(const clang::Type *T);

  RSExportPrimitiveType(RSContext *Context,
                        const llvm::StringRef &Name,
                        DataType DT,
                        DataKind DK,
                        bool Normalized)
      : RSExportType(Context, Name),
        mType(DT),
        mKind(DK),
        mNormalized(Normalized) {
    return;
  }

  virtual const llvm::Type *convertToLLVMType() const;
 public:
  // @T may not be normalized
  static RSExportPrimitiveType *Create(RSContext *Context,
                                       const clang::Type *T,
                                       DataKind DK = DataKindUser);

  static DataType GetRSObjectType(const llvm::StringRef &TypeName);
  static DataType GetRSObjectType(const clang::Type *T);

  static size_t GetSizeInBits(const RSExportPrimitiveType *EPT);

  virtual ExportClass getClass() const;

  inline DataType getType() const { return mType; }
  inline DataKind getKind() const { return mKind; }
  inline bool isRSObjectType() const {
    return ((mType >= DataTypeRSElement) && (mType < DataTypeMax));
  }
};  // RSExportPrimitiveType


class RSExportPointerType : public RSExportType {
  friend class RSExportType;
  friend class RSExportElement;
  friend class RSExportFunc;

 private:
  const RSExportType *mPointeeType;

  RSExportPointerType(RSContext *Context,
                      const llvm::StringRef &Name,
                      const RSExportType *PointeeType)
      : RSExportType(Context, Name),
        mPointeeType(PointeeType) {
    return;
  }

  // @PT was normalized by calling RSExportType::TypeExportable() before calling
  // this.
  static RSExportPointerType *Create(RSContext *Context,
                                     const clang::PointerType *PT,
                                     const llvm::StringRef &TypeName);

  virtual const llvm::Type *convertToLLVMType() const;
 public:
  static const clang::Type *IntegerType;

  virtual ExportClass getClass() const;

  inline const RSExportType *getPointeeType() const { return mPointeeType; }
};  // RSExportPointerType


class RSExportConstantArrayType : public RSExportPrimitiveType {
  friend class RSExportType;
  friend class RSExportElement;
  friend class RSExportFunc;

 private:
  int mNumElement;   // number of element

  RSExportConstantArrayType(RSContext *Context,
                            const llvm::StringRef &Name,
                            DataType DT,
                            DataKind DK,
                            bool Normalized,
                            int NumElement)
      : RSExportPrimitiveType(Context, Name, DT, DK, Normalized),
        mNumElement(NumElement) {
    return;
  }

  static RSExportConstantArrayType *Create(RSContext *Context,
                                           const clang::ConstantArrayType *ECT,
                                           const llvm::StringRef &TypeName,
                                           DataKind DK = DataKindUser,
                                           bool Normalized = false);
  virtual const llvm::Type *convertToLLVMType() const;
 public:
  static llvm::StringRef GetTypeName(const clang::ConstantArrayType *ECT);

  virtual ExportClass getClass() const;

  inline int getNumElement() const { return mNumElement; }
};


class RSExportVectorType : public RSExportPrimitiveType {
  friend class RSExportType;
  friend class RSExportElement;

 private:
  int mNumElement;   // number of element

  RSExportVectorType(RSContext *Context,
                     const llvm::StringRef &Name,
                     DataType DT,
                     DataKind DK,
                     bool Normalized,
                     int NumElement)
      : RSExportPrimitiveType(Context, Name, DT, DK, Normalized),
        mNumElement(NumElement) {
    return;
  }

  // @EVT was normalized by calling RSExportType::TypeExportable() before
  // calling this.
  static RSExportVectorType *Create(RSContext *Context,
                                    const clang::ExtVectorType *EVT,
                                    const llvm::StringRef &TypeName,
                                    DataKind DK = DataKindUser,
                                    bool Normalized = false);

  static const char *VectorTypeNameStore[][3];

  virtual const llvm::Type *convertToLLVMType() const;
 public:
  static llvm::StringRef GetTypeName(const clang::ExtVectorType *EVT);

  virtual ExportClass getClass() const;

  inline int getNumElement() const { return mNumElement; }
};

class RSExportRecordType : public RSExportType {
  friend class RSExportType;
  friend class RSExportElement;
  friend class RSExportFunc;
 public:
  class Field {
   private:
    const RSExportType *mType;
    // Field name
    std::string mName;
    // Link to the struct that contain this field
    const RSExportRecordType *mParent;
    // Index in the container
    unsigned int mIndex;

   public:
    Field(const RSExportType *T,
          const llvm::StringRef &Name,
          const RSExportRecordType *Parent,
          unsigned int Index)
        : mType(T),
          mName(Name.data(), Name.size()),
          mParent(Parent),
          mIndex(Index) {
      return;
    }

    inline const RSExportRecordType *getParent() const { return mParent; }
    inline unsigned int getIndex() const { return mIndex; }
    inline const RSExportType *getType() const { return mType; }
    inline const std::string &getName() const { return mName; }
    size_t getOffsetInParent() const;
  };

  typedef std::list<const Field*>::const_iterator const_field_iterator;

  inline const_field_iterator fields_begin() const {
    return this->mFields.begin();
  }
  inline const_field_iterator fields_end() const {
    return this->mFields.end();
  }

 private:
  std::list<const Field*> mFields;
  bool mIsPacked;
  // Artificial export struct type is not exported by user (and thus it won't
  // get reflected)
  bool mIsArtificial;
  size_t AllocSize;

  RSExportRecordType(RSContext *Context,
                     const llvm::StringRef &Name,
                     bool IsPacked,
                     bool IsArtificial = false)
      : RSExportType(Context, Name),
        mIsPacked(IsPacked),
        mIsArtificial(IsArtificial) {
    return;
  }

  // @RT was normalized by calling RSExportType::TypeExportable() before calling
  // this.
  // @TypeName was retrieved from RSExportType::GetTypeName() before calling
  // this.
  static RSExportRecordType *Create(RSContext *Context,
                                    const clang::RecordType *RT,
                                    const llvm::StringRef &TypeName,
                                    bool mIsArtificial = false);

  virtual const llvm::Type *convertToLLVMType() const;
 public:
  virtual ExportClass getClass() const;

  inline bool isPacked() const { return mIsPacked; }
  inline bool isArtificial() const { return mIsArtificial; }
  inline size_t getAllocSize() const { return AllocSize; }

  ~RSExportRecordType() {
    for (std::list<const Field*>::iterator I = mFields.begin(),
             E = mFields.end();
         I != E;
         I++)
      if (*I != NULL)
        delete *I;
    return;
  }
};  // RSExportRecordType

}   // namespace slang

#endif  // _SLANG_COMPILER_RS_EXPORT_TYPE_H