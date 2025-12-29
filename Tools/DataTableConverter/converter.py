#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
UE5 数据表转换工具
功能：将策划的 CSV/Excel 文件转换为 UE DataTable 兼容格式

作者：自动生成
日期：2025-12-29
"""

import pandas as pd
import json
import sys
import os
from pathlib import Path
from typing import Dict, List, Optional

# Windows 命令行编码修复
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')

class DataTableConverter:
    """数据表转换器"""

    def __init__(self, config_path: str = "config.json"):
        """初始化转换器"""
        self.config = self._load_config(config_path)
        self.errors: List[str] = []
        self.warnings: List[str] = []

    def _load_config(self, config_path: str) -> dict:
        """加载配置文件"""
        if not os.path.exists(config_path):
            print(f"⚠️  配置文件不存在: {config_path}")
            print(f"📝 使用默认配置")
            return self._get_default_config()

        with open(config_path, 'r', encoding='utf-8') as f:
            return json.load(f)

    def _get_default_config(self) -> dict:
        """获取默认配置"""
        return {
            "source_path": "../../SourceData",
            "output_path": "../../Output/CSV",
            "tables": {
                "items": {
                    "source": "Items.csv",
                    "output": "DT_ItemDefinition.csv",
                    "primary_key": "ItemID"
                },
                "containers": {
                    "source": "Containers.csv",
                    "output": "DT_ContainerConfig.csv",
                    "primary_key": "ContainerID"
                },
                "economy": {
                    "source": "Economy.csv",
                    "output": "DT_EconomyConfig.csv",
                    "primary_key": "ItemID"
                }
            }
        }

    def convert_table(self, table_name: str) -> bool:
        """转换单个数据表"""
        if table_name not in self.config["tables"]:
            print(f"❌ 未知的表名: {table_name}")
            return False

        table_config = self.config["tables"][table_name]
        source_path = Path(self.config["source_path"]) / table_config["source"]
        output_path = Path(self.config["output_path"]) / table_config["output"]

        print(f"\n{'='*60}")
        print(f"📊 转换数据表: {table_name}")
        print(f"{'='*60}")
        print(f"📂 源文件: {source_path}")
        print(f"📂 输出文件: {output_path}")

        # 1. 读取源文件
        if not source_path.exists():
            print(f"❌ 源文件不存在: {source_path}")
            return False

        try:
            df = pd.read_csv(source_path, encoding='utf-8-sig')
            print(f"✅ 读取成功: {len(df)} 行数据")
        except Exception as e:
            print(f"❌ 读取失败: {e}")
            return False

        # 2. 数据验证
        if not self._validate_table(table_name, df, table_config):
            return False

        # 3. 数据转换
        df = self._transform_table(table_name, df)

        # 4. 输出文件
        try:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            df.to_csv(output_path, index=False, encoding='utf-8-sig')
            print(f"✅ 导出成功: {output_path}")
            return True
        except Exception as e:
            print(f"❌ 导出失败: {e}")
            return False

    def _validate_table(self, table_name: str, df: pd.DataFrame, config: dict) -> bool:
        """验证数据表"""
        print(f"\n🔍 数据验证...")
        self.errors.clear()
        self.warnings.clear()

        # 检查主键
        primary_key = config.get("primary_key")
        if primary_key:
            if primary_key not in df.columns:
                self.errors.append(f"缺少主键列: {primary_key}")
            else:
                # 检查主键唯一性
                duplicates = df[df.duplicated(primary_key, keep=False)]
                if not duplicates.empty:
                    duplicate_ids = duplicates[primary_key].unique().tolist()
                    self.errors.append(f"主键重复: {duplicate_ids}")

        # 物品表特殊验证
        if table_name == "items":
            self._validate_items(df)

        # 容器表特殊验证
        if table_name == "containers":
            self._validate_containers(df)

        # 输出验证结果
        if self.warnings:
            print(f"\n⚠️  警告 ({len(self.warnings)} 个):")
            for warning in self.warnings:
                print(f"   - {warning}")

        if self.errors:
            print(f"\n❌ 错误 ({len(self.errors)} 个):")
            for error in self.errors:
                print(f"   - {error}")
            return False

        print(f"✅ 验证通过")
        return True

    def _validate_items(self, df: pd.DataFrame):
        """验证物品表"""
        # 检查必填字段
        required_fields = ['ItemID', 'ItemName', 'ItemType', 'SizeX', 'SizeY']
        for field in required_fields:
            if field not in df.columns:
                self.errors.append(f"缺少必填字段: {field}")
                return

        # 检查尺寸合法性
        invalid_size = df[(df['SizeX'] <= 0) | (df['SizeY'] <= 0)]
        if not invalid_size.empty:
            invalid_ids = invalid_size['ItemID'].tolist()
            self.errors.append(f"物品尺寸必须 > 0: {invalid_ids}")

        # 检查价格合法性
        if 'BasePrice' in df.columns:
            negative_price = df[df['BasePrice'] < 0]
            if not negative_price.empty:
                invalid_ids = negative_price['ItemID'].tolist()
                self.errors.append(f"物品价格不能为负: {invalid_ids}")

        # 检查容器配置
        containers = df[df['IsContainer'] == True]
        for idx, row in containers.iterrows():
            if pd.isna(row.get('ContainerGridSizeX')) or pd.isna(row.get('ContainerGridSizeY')):
                self.errors.append(f"容器 {row['ItemID']} 未定义网格尺寸")

    def _validate_containers(self, df: pd.DataFrame):
        """验证容器表"""
        # 检查网格尺寸
        invalid_grid = df[(df['GridWidth'] <= 0) | (df['GridHeight'] <= 0)]
        if not invalid_grid.empty:
            invalid_ids = invalid_grid['ContainerID'].tolist()
            self.errors.append(f"容器网格尺寸必须 > 0: {invalid_ids}")

    def _transform_table(self, table_name: str, df: pd.DataFrame) -> pd.DataFrame:
        """转换数据表为 UE 格式"""
        print(f"\n🔄 数据转换...")

        # 填充默认值
        if 'CanRotate' in df.columns:
            df['CanRotate'] = df['CanRotate'].fillna(True)

        if 'MaxStackSize' in df.columns:
            df['MaxStackSize'] = df['MaxStackSize'].fillna(1).astype(int)

        if 'BaseWeight' in df.columns:
            df['BaseWeight'] = df['BaseWeight'].fillna(0.0)

        if 'BasePrice' in df.columns:
            df['BasePrice'] = df['BasePrice'].fillna(0).astype(int)

        if 'IsContainer' in df.columns:
            df['IsContainer'] = df['IsContainer'].fillna(False)

        if 'ContainerGridSizeX' in df.columns:
            df['ContainerGridSizeX'] = df['ContainerGridSizeX'].fillna(0).astype(int)

        if 'ContainerGridSizeY' in df.columns:
            df['ContainerGridSizeY'] = df['ContainerGridSizeY'].fillna(0).astype(int)

        # 转换布尔值为 UE 格式
        bool_columns = df.select_dtypes(include=['bool']).columns
        for col in bool_columns:
            df[col] = df[col].apply(lambda x: 'True' if x else 'False')

        # 处理空字符串
        df = df.fillna('')

        print(f"✅ 转换完成")
        return df

    def convert_all(self) -> bool:
        """转换所有数据表"""
        print(f"\n{'#'*60}")
        print(f"# UE5 数据表转换工具")
        print(f"{'#'*60}")

        success_count = 0
        total_count = len(self.config["tables"])

        for table_name in self.config["tables"].keys():
            if self.convert_table(table_name):
                success_count += 1

        print(f"\n{'='*60}")
        print(f"📊 转换完成: {success_count}/{total_count} 个表成功")
        print(f"{'='*60}")

        return success_count == total_count

def main():
    """命令行入口"""
    import argparse

    parser = argparse.ArgumentParser(description='UE5 数据表转换工具')
    parser.add_argument('--config', default='config.json', help='配置文件路径')
    parser.add_argument('--table', help='指定转换的表名（不指定则转换所有表）')
    parser.add_argument('--validate-only', action='store_true', help='仅验证不导出')

    args = parser.parse_args()

    # 创建转换器
    converter = DataTableConverter(args.config)

    # 执行转换
    if args.table:
        success = converter.convert_table(args.table)
    else:
        success = converter.convert_all()

    # 返回退出码
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
