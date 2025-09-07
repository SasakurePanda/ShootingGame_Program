#include "Skydome.h"
#include "Renderer.h"
#include "TextureManager.h"
#include "Application.h"
#include "ICameraViewProvider.h"

SkyDome::SkyDome(const std::string& texPath)
{
	if (!texPath.empty())
	{
		//texPathが空でなければ、ロードする
		m_texture.Attach(TextureManager::Load(texPath));
	}
}

void SkyDome::Initialize()
{
//-----------ローポリの球をPrimitiveを使って作る-----------
//---------------スカイドームなので裏向き------------
	//renderクラスからドライブコンテキストを取ってくる
	ID3D11Device* dev = Renderer::GetDevice();

	//16*8で少なめに
	m_primitive.CreateSphere(dev, m_radius, 16, 8);
}

void SkyDome::Update(float dt)
{
	if (m_cameraProvider)
	{
		//カメラのワールド座標を引っ張ってくる
		Vector3 camPos = m_cameraProvider->GetPosition();
		//セットする
		SetPosition(camPos);

	}
	// カメラの位置に追従（プレイヤーをターゲットにしている CameraComponent があればそちらの位置を使う）
	// ここでは Application::GetWindow() は関係ないので、Scene 側からカメラ参照を渡すのが良い。
	// 仮にカメラが m_owner の外にあり、Scene が渡しているなら GetOwner() は不要。
	// 代替：カメラをグローバルに取得する仕組みがあればそれを使う。
	// 例（あなたのコードに合わせて）:
	// if (m_owner) m_owner->SetPosition(cameraPos);
	// 簡易：位置を(0,0,0)にしておく（Scene 側で SetPosition(cameraPos) を呼ぶ想定）
}

void SkyDome::Draw(float alpha)
{
	//深度とカリングをオフにして描画
	Renderer::SetDepthEnable(false);  //深度テスト（Zバッファ）を無効
	Renderer::SetBlendState(BS_NONE); //ブレンドステートをセット(半透明合成無し)
	Renderer::DisableCulling(false);  //面の除外（カリング）を無効

	//カメラに追従させる
	Matrix4x4 world = GetTransform().GetMatrix();
	Renderer::SetWorldMatrix(&world);

	if (m_texture)
	{
		Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
	}

	//描画関数
	//(PrimitiveのDrawにRendererのデバイスコンテキストを渡す)
	m_primitive.Draw(Renderer::GetDeviceContext());

	//後処理
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);

	//前の設定に戻す
	Renderer::SetDepthEnable(true);			//深度テスト（Zバッファ）を有効
	Renderer::SetBlendState(BS_ALPHABLEND);	//ブレンドステートをセット(半透明合成)
}