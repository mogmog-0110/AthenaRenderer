#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import re

# Math.hファイルを読み取り、文字化けを修正
math_file_path = 'AthenaCore/include/Athena/Utils/Math.h'

# 文字化け文字列と正しい文字列の対応表
replacements = {
    # DirectXコメント関連
    "DirectX 12で、C++側は行優先、HLSL側は列優先として扱う。": "DirectX 12では、C++側は行優先、HLSL側は列優先として扱う。",
    "そのため、シェーダーに渡す前に転置が必要な場合がある。": "そのため、シェーダーに渡す前に転置が必要な場合がある。",
    
    # 問題のある文字化け文字列を直接置換
    "転置（HLSL用に列優先に変換）": "転置（HLSL用に列優先に変換）",
    "行列式（3x3余因子）": "行列式（3x3余因子）",
    "行列式": "行列式",
    "逆行列": "逆行列", 
    "逆行列が存在しない場合は単位行列を返す": "逆行列が存在しない場合は単位行列を返す",
    "平行移動行列": "平行移動行列",
    "スケール行列": "スケール行列",
    "X軸回転行列": "X軸回転行列",
    "Y軸回転行列": "Y軸回転行列",
    "Z軸回転行列": "Z軸回転行列",
    "任意軸回転行列（Rodriguesの回転公式）": "任意軸回転行列（Rodriguesの回転公式）",
    "ビュー行列（右手座標系）": "ビュー行列（右手座標系）",
    "前方向": "前方向",
    "右方向": "右方向",
    "上方向": "上方向",
    "ビュー行列（左手座標系）": "ビュー行列（左手座標系）", 
    "前方向（右手系は逆）": "前方向（右手系は逆）",
    "デフォルトのLookAt（右手座標系）": "デフォルトのLookAt（右手座標系）",
    "透視投影行列（右手座標系）": "透視投影行列（右手座標系）",
    "透視投影行列（左手座標系）": "透視投影行列（左手座標系）",
    "デフォルトのPerspective（右手座標系）": "デフォルトのPerspective（右手座標系）",
    "正射影行列（右手座標系）": "正射影行列（右手座標系）",
    "正射影行列（左手座標系）": "正射影行列（左手座標系）",
    "オフセンター正射影（右手座標系）": "オフセンター正射影（右手座標系）",
    "オフセンター正射影（左手座標系）": "オフセンター正射影（左手座標系）",
    "定数": "定数",
    "度をラジアンに変換": "度をラジアンに変換",
    "ラジアンを度に変換": "ラジアンを度に変換", 
    "値のクランプ": "値のクランプ",
    "線形補間": "線形補間",
    "滑らか補間（Smoothstep）": "滑らか補間（Smoothstep）"
}

try:
    # ファイルを読み込み（異なるエンコーディングを試行）
    content = ""
    encodings = ['utf-8', 'utf-8-sig', 'cp932', 'shift_jis', 'iso-2022-jp']
    
    for encoding in encodings:
        try:
            with open(math_file_path, 'r', encoding=encoding) as f:
                content = f.read()
            print(f"Successfully read file with encoding: {encoding}")
            break
        except UnicodeDecodeError:
            continue
    
    if not content:
        print("Failed to read file with any encoding")
        sys.exit(1)
    
    # 文字化け文字列を検索して表示
    corrupted_patterns = [
        r'DirectX 12[^\n]*AC\+\+[^\n]*HLSL[^\n]*',
        r'[^\n]*シェーダー[^\n]*転置[^\n]*',
        r'[^\w\s\(\)（）]*転置[^\n]*',
        r'[^\w\s\(\)（）]*行列式[^\n]*',
        r'[^\w\s\(\)（）]*逆行列[^\n]*',
        r'[^\w\s\(\)（）]*平行移動[^\n]*',
        r'[^\w\s\(\)（）]*スケール[^\n]*',
        r'[^\w\s\(\)（）]*回転[^\n]*',
        r'[^\w\s\(\)（）]*ビュー[^\n]*',
        r'[^\w\s\(\)（）]*投影[^\n]*',
        r'[^\w\s\(\)（）]*正射影[^\n]*',
        r'[^\w\s\(\)（）]*定数[^\n]*',
        r'[^\w\s\(\)（）]*度を[^\n]*',
        r'[^\w\s\(\)（）]*ラジアン[^\n]*',
        r'[^\w\s\(\)（）]*クランプ[^\n]*',
        r'[^\w\s\(\)（）]*線形[^\n]*',
        r'[^\w\s\(\)（）]*滑らか[^\n]*'
    ]
    
    print("Found potentially corrupted lines:")
    for pattern in corrupted_patterns:
        matches = re.findall(pattern, content)
        for match in matches:
            if any(ord(c) > 127 and c not in 'あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをんアイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンー' for c in match):
                print(f"  {match}")
    
    print("\nMath.h appears to have character encoding issues that need manual fixing.")
    print("Please use a text editor to manually correct the corrupted Japanese text.")
    
except Exception as e:
    print(f"Error: {e}")