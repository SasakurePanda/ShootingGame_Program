#include "MiniMapComponent.h"
#include "Renderer.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;
/// <summary>
/// ワールド座標を基準にミニマップ上の座標に変換する
/// </summary>
/// <param name="worldPos">オブジェクトのワールド座標</param>
/// <param name="playerPos">プレイヤーのワールド座標</param>
/// <param name="playeryaw">プレイヤーの</param>
/// <returns></returns>
Vector2 MiniMapComponent::WorldToMiniMap(const Vector3& worldPos,
										 const Vector3& playerPos,
										 float playerYaw) const
{
	//ミニマップの中心座標を計算
	Vector2 center = { m_screenPos.x + (m_size.x * 0.5f),
					   m_screenPos.y + (m_size.y * 0.5f) }; 
	
	float radiusWorld = m_coverageRadius;

	//オブジェクトのワールド座標とプレイヤーのワールド座標の差分を計算
	float radius = m_coverageRadius;
	if(radius <= 0.01f)
	{
		radius = 0.01f;
	}

	Vector2 rel = {worldPos.x - playerPos.x,
				   worldPos.z - playerPos.z };

	//プレイヤーの向きに合わせて回転させる
	if (m_rotateWithPlayer)
	{
		float yawRad = playerYaw;

		float cosYaw = std::cos(yawRad);
		float sinYaw = std::sin(yawRad);

		float rx = (rel.x * cosYaw) - (rel.y * sinYaw);
		float rz = (rel.x * sinYaw) + (rel.y * cosYaw);

		rel.x = rx;
		rel.y = rz;
	}

	//プレイヤーからの相対的な距離を正規化する
	//現在地からPlayerへのベクトル÷ミニマップのカバー範囲の半径
	Vector2 norm = { rel.x / radiusWorld , rel.y / radiusWorld };

	float lenSq = (norm.x * norm.x) + (norm.y * norm.y);

	if (lenSq > 1.0f)
	{
		float len = std::sqrt(lenSq);
		if (lenSq > 0.0001f)
		{
			norm.x /= len;
			norm.y /= len;
		}
	}

	//
	float halfWidth = m_size.x * 0.5f;
	float halfHeight = m_size.y * 0.5f;
	float radiusPx = std::min(halfWidth, halfHeight);

	float iconRadiusPx = m_iconSizePx * 0.5f;
	float marginPx = 2.0f; // 好みで調整
	radiusPx -= (iconRadiusPx + marginPx);

	if (radiusPx < 1.0f)
	{
		radiusPx = 1.0f;
	}

	Vector2 pixel;
	pixel.x = center.x + (norm.x * radiusPx);

	// Zを「上」にしたいならマイナス。上下が逆ならここを + に変える
	pixel.y = center.y - (norm.y * radiusPx);

	return pixel;
}

void MiniMapComponent::Draw(float alpha)
{
	if (!m_player) { return; }

	GameObject* owner = GetOwner();

	if (!owner) { return; }

	Vector3 playerPos = m_player->GetPosition();
	Vector3 playerRot = m_player->GetRotation();
	float playerYaw = playerRot.y;

	//背景描画
	if (m_backgroundSRV)
	{
		Renderer::DrawTexture(m_backgroundSRV, m_screenPos, m_size);

	}
	
	//プレイヤーアイコン描画
	Vector2 iconSize = { m_iconSizePx, m_iconSizePx };

	if (m_playerIconSRV)
	{
		Vector2 center = { m_screenPos.x + m_size.x * 0.5f, m_screenPos.y + m_size.y * 0.5f };
		Vector2 drawPos = { center.x - iconSize.x * 0.5f, center.y - iconSize.y * 0.5f };
		Renderer::DrawTexture(m_playerIconSRV, drawPos, iconSize);
	}

	//建物アイコン描画
	if (m_buildingIconSRV)
	{
		for (GameObject* building : m_buildings)
		{
			if (!building) { continue; }

			Vector3 bPos = building->GetPosition();
			Vector2 bPixel = WorldToMiniMap(bPos, playerPos, playerYaw);

			Vector2 drawPos = { bPixel.x - (iconSize.x * 0.5f), bPixel.y - (iconSize.y * 0.5f) };
			Renderer::DrawTexture(m_buildingIconSRV, drawPos, iconSize);
		}
	}

	//敵アイコン描画
	if (m_enemyIconSRV)
	{
		for (GameObject* enemy : m_enemies)
		{
			if (!enemy) { continue; }

			Vector3 ePos = enemy->GetPosition();
			Vector2 ePixel = WorldToMiniMap(ePos, playerPos, playerYaw);

			Vector2 drawPos = { ePixel.x - (iconSize.x * 0.5f), ePixel.y - (iconSize.y * 0.5f) };
			Renderer::DrawTexture(m_enemyIconSRV, drawPos, iconSize);
		}
	}
}