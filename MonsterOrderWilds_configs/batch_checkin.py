"""
批量补签脚本 v2 - 通用版
功能：补签所有有累计打卡天数的用户的缺失日期，使打卡连续到今天
特点：
1. 保留原始打卡记录，只补签缺失日期
2. 今天日期动态获取
3. 可重复运行，幂等性
"""
import sqlite3
import os
from datetime import datetime, timedelta

DB_PATH = r'D:\VisualStudioProjects\JonysandMHDanmuTools\MonsterOrderWilds_configs\captain_profiles.db'

def get_today():
    """动态获取今天日期 YYYYMMDD"""
    now = datetime.now()
    return now.year * 10000 + now.month * 100 + now.day

def get_timestamp():
    """当前时间戳(ms)"""
    return int(datetime.now().timestamp() * 1000)

def date_to_int(dt):
    return dt.year * 10000 + dt.month * 100 + dt.day

def int_to_date(d):
    year = d // 10000
    month = (d % 10000) // 100
    day = d % 100
    return datetime(year, month, day)

def next_date(d):
    dt = int_to_date(d) + timedelta(days=1)
    return date_to_int(dt)

def main():
    TODAY = get_today()
    TIMESTAMP = get_timestamp()
    
    print(f"今天日期: {TODAY}")
    print(f"数据库路径: {DB_PATH}")
    
    if not os.path.exists(DB_PATH):
        print("错误: 数据库文件不存在")
        return
    
    db = sqlite3.connect(DB_PATH)
    db.execute("PRAGMA journal_mode=WAL")
    cursor = db.cursor()

    # 获取所有有累计打卡天数的用户
    cursor.execute("""
        SELECT uid, username, last_checkin_date, continuous_days, cumulative_days 
        FROM user_profiles 
        WHERE cumulative_days > 0
    """)
    users = cursor.fetchall()
    print(f"共找到 {len(users)} 个需要补签的用户")

    total_inserted = 0
    users_updated = 0
    users_skipped = 0

    for uid, username, last_checkin, continuous, cumulative in users:
        # 获取用户的所有打卡日期
        cursor.execute("""
            SELECT checkin_date FROM checkin_records 
            WHERE uid = ? 
            ORDER BY checkin_date ASC
        """, (uid,))
        existing_dates = set(row[0] for row in cursor.fetchall())

        # 如果没有打卡记录，从 cumulative_days 计算起始日期
        if not existing_dates:
            # 从今天往前推 cumulative-1 天作为起始
            start_date = int_to_date(TODAY)
            for i in range(cumulative - 1):
                start_date -= timedelta(days=1)
            min_date = date_to_int(start_date)
        else:
            min_date = min(existing_dates)

        # 确定结束日期：取 last_checkin 和 TODAY 的较大值
        end_date = max(last_checkin, TODAY) if last_checkin > 0 else TODAY

        # 从 min_date 到 end_date，生成所有日期
        all_dates = set()
        current = min_date
        while current <= end_date:
            all_dates.add(current)
            current = next_date(current)

        # 找出缺失的日期
        missing_dates = sorted(all_dates - existing_dates)

        # 补签缺失的日期
        for checkin_date in missing_dates:
            cursor.execute("""
                INSERT OR IGNORE INTO checkin_records (uid, checkin_date, created_at, username)
                VALUES (?, ?, ?, ?)
            """, (uid, checkin_date, TIMESTAMP, username))
            total_inserted += 1

        # 计算新的连续天数
        new_continuous = cumulative
        new_last_checkin = max(end_date, TODAY) if missing_dates else last_checkin

        # 更新 user_profiles
        if new_continuous != continuous or new_last_checkin != last_checkin:
            cursor.execute("""
                UPDATE user_profiles 
                SET last_checkin_date = ?, continuous_days = ?, updated_at = ?
                WHERE uid = ?
            """, (new_last_checkin, new_continuous, TIMESTAMP, uid))
            users_updated += 1
        
        if missing_dates:
            print(f"  {username}: 补签 {len(missing_dates)} 天")
        else:
            users_skipped += 1

    db.commit()

    print(f"\n=== 操作完成 ===")
    print(f"总用户数: {len(users)}")
    print(f"补签用户数: {len(users) - users_skipped}")
    print(f"跳过用户数: {users_skipped} (已连续到今天)")
    print(f"插入打卡记录数: {total_inserted}")

    # 验证结果
    print(f"\n=== 验证结果 ===")
    cursor.execute("""
        SELECT COUNT(*) FROM user_profiles 
        WHERE cumulative_days > 0 AND continuous_days != cumulative_days
    """)
    mismatch = cursor.fetchone()[0]
    if mismatch:
        print(f"警告: {mismatch} 个用户 continuous != cumulative")
    else:
        print("所有用户 continuous_days == cumulative_days OK")

    cursor.execute("""
        SELECT COUNT(*) FROM user_profiles p
        WHERE p.cumulative_days > 0 
        AND NOT EXISTS (
            SELECT 1 FROM checkin_records r 
            WHERE r.uid = p.uid 
            GROUP BY r.uid 
            HAVING COUNT(*) = p.cumulative_days
        )
    """)
    record_mismatch = cursor.fetchone()[0]
    if record_mismatch:
        print(f"警告: {record_mismatch} 个用户打卡记录数 != cumulative_days")
    else:
        print("所有用户打卡记录数 == cumulative_days OK")

    # 显示统计
    print(f"\n=== 打卡分布 ===")
    cursor.execute("""
        SELECT cumulative_days, COUNT(*) as cnt 
        FROM user_profiles 
        WHERE cumulative_days > 0 
        GROUP BY cumulative_days 
        ORDER BY cumulative_days
    """)
    for cum, cnt in cursor.fetchall():
        print(f"  cumulative={cum}: {cnt} 人")

    db.close()
    print("\n验证完成")

if __name__ == "__main__":
    main()
