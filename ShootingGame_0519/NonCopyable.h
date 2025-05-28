#pragma once
//-------------------------NonCopyableクラス-------------------------
//コピー操作（コピーコンストラクタおよび代入演算子）を禁止できるよう、
//なるクラス
//-------------------------------------------------------------------
class NonCopyable
{
public:
    //デフォルトコンストラクタ
    NonCopyable() = default;   

    //デフォルトデストラクタ
    ~NonCopyable() = default;

    //クラスの複製を防ぎリソースの重複管理を防止する関数
    NonCopyable(const NonCopyable&) = delete;      
    
    //別のオブジェクトへの上書きも防ぐ関数
    NonCopyable& operator=(const NonCopyable&) = delete;
};
