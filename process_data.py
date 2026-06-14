import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import os

# 1. 파일 경로 설정
csv_path = "datalogs/sensor_log_20260530_lap.csv"
stats_path = "datalogs/sensor_stats_20260530_lap.txt"
chart_path = "datalogs/sensor_chart_20260530_lap.png"
html_path = "datalogs/sensor_log_formatted_20260530_lap.html"

# CSV 파일 존재 확인
if not os.path.exists(csv_path):
    print(f"[오류] {csv_path} 파일이 존재하지 않습니다. 먼저 올바른 경로를 확인해 주세요.")
    exit(1)

print("[SYSTEM] CSV 파일 로드 및 결측치 처리 시작...")
# CSV 로드 및 데이터 정리 (nan 결측치 처리)
df = pd.read_csv(csv_path)
df['Sound_RMS'] = pd.to_numeric(df['Sound_RMS'], errors='coerce').fillna(0.0)
df['Sound_dB'] = pd.to_numeric(df['Sound_dB'], errors='coerce').fillna(0.0)
df['Temperature'] = pd.to_numeric(df['Temperature'], errors='coerce').fillna(0.0)
df['Humidity'] = pd.to_numeric(df['Humidity'], errors='coerce').fillna(0.0)
df['Motion'] = pd.to_numeric(df['Motion'], errors='coerce').fillna(0).astype(int)
df['BLE_Count'] = pd.to_numeric(df['BLE_Count'], errors='coerce').fillna(0.0).astype(float)

# 시간 컬럼 파싱
df['Timestamp'] = pd.to_datetime(df['Timestamp'])

# 2. 통계값 산출 및 텍스트 파일 저장
total_records = len(df)
duration_sec = (df['Timestamp'].max() - df['Timestamp'].min()).total_seconds()
duration_str = f"{int(duration_sec // 3600)}시간 {int((duration_sec % 3600) // 60)}분 {int(duration_sec % 60)}초"

avg_temp = df['Temperature'].mean()
max_temp = df['Temperature'].max()
min_temp = df['Temperature'].min()

avg_hum = df['Humidity'].mean()
max_hum = df['Humidity'].max()
min_hum = df['Humidity'].min()

avg_db = df['Sound_dB'].mean()
max_db = df['Sound_dB'].max()
min_db = df['Sound_dB'].min()

motion_ratio = (df['Motion'].sum() / total_records) * 100 if total_records > 0 else 0
avg_ble = df['BLE_Count'].mean()

acoustic_counts = df['Acoustic_Category'].value_counts()
occupancy_counts = df['Occupancy_Status'].value_counts()

# 통계 텍스트 작성
stats_content = f"""==================================================
        CampusSpot Sensor Log Analysis Report
==================================================
1. 기본 수집 정보
- 분석 대상 파일: {csv_path}
- 총 데이터 레코드 수: {total_records}개
- 데이터 수집 기간: {df['Timestamp'].min()} ~ {df['Timestamp'].max()} (총 {duration_str})

2. 센서별 통계 정보
[온도 (Temperature)]
- 평균 온도: {avg_temp:.1f} °C
- 최고 온도: {max_temp:.1f} °C
- 최저 온도: {min_temp:.1f} °C

[습도 (Humidity)]
- 평균 습도: {avg_hum:.1f} %
- 최고 습도: {max_hum:.1f} %
- 최저 습도: {min_hum:.1f} %

[소음도 (Sound Level)]
- 평균 소음: {avg_db:.1f} dB
- 최고 소음: {max_db:.1f} dB
- 최저 소음: {min_db:.1f} dB

[활동 및 기기 수 (Activity & Devices)]
- 움직임 감지 비율: {motion_ratio:.1f} % ({df['Motion'].sum()} / {total_records} 초)
- 평균 주변 BLE 기기 수: {avg_ble:.1f} 대 (최대 {df['BLE_Count'].max()} 대)

3. 상태 분류 분포
[음향 정체성 분류 (Acoustic Category)]
"""
for cat, count in acoustic_counts.items():
    stats_content += f"- {cat:<20}: {count}개 ({count/total_records*100:.1f}%)\n"

stats_content += "\n[공간 점유 상태 분류 (Occupancy Status)]\n"
for occ, count in occupancy_counts.items():
    stats_content += f"- {occ:<20}: {count}개 ({count/total_records*100:.1f}%)\n"

stats_content += "=================================================="

with open(stats_path, "w", encoding="utf-8") as f:
    f.write(stats_content)

print(f"[완료] 1. 통계 분석 보고서 생성 완료: {stats_path}")

# 3. 차트 생성 및 저장 (sound_dB, Temp/Hum)
print("[SYSTEM] 시각화 차트 이미지 그리는 중...")
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

# 첫번째 그래프: 소음 데시벨
ax1.plot(df['Timestamp'], df['Sound_dB'], label='Sound Level (dB)', color='#2ca02c', linewidth=1.2)
# 움직임 감지 마킹 (빨간색 점)
motion_indices = df['Motion'] == 1
if motion_indices.any():
    ax1.scatter(df.loc[motion_indices, 'Timestamp'], df.loc[motion_indices, 'Sound_dB'], 
                color='red', label='Motion Detected', s=12, zorder=5)

ax1.set_title("Real-Time Sound Decibels (dB) & Active Motion", fontsize=12, fontweight='bold')
ax1.set_ylabel("Decibel (dB)", fontsize=10)
ax1.grid(True, linestyle='--', alpha=0.5)
ax1.legend(loc='upper right')
ax1.set_ylim(0, 100)

# 두번째 그래프: 온습도 (이중 축)
color = '#ff7f0e'
ax2.plot(df['Timestamp'], df['Temperature'], label='Temperature (°C)', color=color, linewidth=1.5)
ax2.set_ylabel('Temperature (°C)', color=color, fontsize=10)
ax2.tick_params(axis='y', labelcolor=color)
ax2.grid(True, linestyle='--', alpha=0.5)

ax2_twin = ax2.twinx()
color = '#1f77b4'
ax2_twin.plot(df['Timestamp'], df['Humidity'], label='Humidity (%)', color=color, linewidth=1.5, linestyle=':')
ax2_twin.set_ylabel('Humidity (%)', color=color, fontsize=10)
ax2_twin.tick_params(axis='y', labelcolor=color)

# 축 레이블 합치기
lines1, labels1 = ax2.get_legend_handles_labels()
lines2, labels2 = ax2_twin.get_legend_handles_labels()
ax2.legend(lines1 + lines2, labels1 + labels2, loc='upper right')

ax2.set_title("Environmental Temperature & Humidity Trends", fontsize=12, fontweight='bold')
ax2.set_xlabel("Timeline", fontsize=10)

plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.gcf().autofmt_xdate()
plt.tight_layout()
plt.savefig(chart_path, dpi=300)
plt.close()
print(f"[완료] 2. 시각화 차트 이미지 생성 완료: {chart_path}")

# 4. 조건부 서식이 적용된 대화형 HTML 생성
print("[SYSTEM] 조건부 서식이 지정된 HTML 대시보드 생성 중...")
# 색상 매핑 함수
def get_db_color(db):
    if db < 40: return "background-color: #e2f0d9; color: #385723;"  # 연한 녹색 (조용함)
    elif db < 60: return "background-color: #fff2cc; color: #7f6000;" # 연한 노란색 (보통)
    else: return "background-color: #fce4d6; color: #c65911;"         # 연한 주황/적색 (시끄러움)

def get_motion_color(motion):
    if motion == 1: return "background-color: #f8cbad; color: #c65911; font-weight: bold;" # 움직임 감지
    return "color: #7f7f7f;"

def get_occ_color(status):
    status_lower = status.lower()
    if "vacant" in status_lower: return "background-color: #f2f2f2; color: #595959;"
    elif "static" in status_lower: return "background-color: #e2f0d9; color: #385723;"
    else: return "background-color: #fff2cc; color: #7f6000;"

html_rows = []
for idx, row in df.iterrows():
    db_style = get_db_color(row['Sound_dB'])
    motion_style = get_motion_color(row['Motion'])
    occ_style = get_occ_color(row['Occupancy_Status'])
    
    motion_text = "YES" if row['Motion'] == 1 else "NO"
    
    html_rows.append(f"""
    <tr>
        <td>{row['Timestamp'].strftime('%Y-%m-%d %H:%M:%S')}</td>
        <td>{row['Temperature']:.1f} °C</td>
        <td>{row['Humidity']:.1f} %</td>
        <td style="{motion_style}">{motion_text}</td>
        <td>{row['Sound_RMS']:.6f}</td>
        <td style="{db_style}">{row['Sound_dB']:.1f} dB</td>
        <td>{row['Acoustic_Category']}</td>
        <td>{row['BLE_Count']}</td>
        <td style="{occ_style}">{row['Occupancy_Status']}</td>
    </tr>
    """)

rows_joined = "\n".join(html_rows)

html_content = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>StudySpot Sensor Log Viewer</title>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 20px;
            background-color: #f5f6f8;
            color: #333;
        }}
        .header {{
            background: linear-gradient(135deg, #1f77b4, #2ca02c);
            color: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }}
        .summary-cards {{
            display: flex;
            gap: 15px;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }}
        .card {{
            background: white;
            padding: 15px;
            border-radius: 8px;
            flex: 1;
            min-width: 150px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.05);
            border-left: 5px solid #1f77b4;
        }}
        .card.sound {{ border-left-color: #2ca02c; }}
        .card.motion {{ border-left-color: #d62728; }}
        .card-title {{
            font-size: 12px;
            color: #888;
            text-transform: uppercase;
        }}
        .card-value {{
            font-size: 24px;
            font-weight: bold;
            margin-top: 5px;
        }}
        .chart-container {{
            background: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.05);
            text-align: center;
        }}
        .chart-container img {{
            max-width: 100%;
            height: auto;
            border-radius: 5px;
        }}
        .search-bar {{
            margin-bottom: 15px;
            display: flex;
            gap: 10px;
        }}
        .search-bar input {{
            padding: 10px;
            width: 300px;
            border: 1px solid #ccc;
            border-radius: 5px;
            font-size: 14px;
        }}
        .table-container {{
            background: white;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0 4px 6px rgba(0,0,0,0.05);
        }}
        table {{
            width: 100%;
            border-collapse: collapse;
            font-size: 14px;
        }}
        th, td {{
            padding: 12px 15px;
            text-align: left;
            border-bottom: 1px solid #eee;
        }}
        th {{
            background-color: #f8f9fa;
            font-weight: bold;
            color: #555;
            cursor: pointer;
        }}
        tr:hover {{
            background-color: #f1f3f5;
        }}
    </style>
    <script>
        function filterTable() {{
            var input, filter, table, tr, td, i, txtValue;
            input = document.getElementById("searchInput");
            filter = input.value.toUpperCase();
            table = document.getElementById("logTable");
            tr = table.getElementsByTagName("tr");
            
            for (i = 1; i < tr.length; i++) {{
                tr[i].style.display = "none";
                tds = tr[i].getElementsByTagName("td");
                for (var j = 0; j < tds.length; j++) {{
                    if (tds[j]) {{
                        txtValue = tds[j].textContent || tds[j].innerText;
                        if (txtValue.toUpperCase().indexOf(filter) > -1) {{
                            tr[i].style.display = "";
                            break;
                        }}
                    }}
                }}
            }}
        }}
    </script>
</head>
<body>
    <div class="header">
        <h1>StudySpot Sensor Log Dashboard</h1>
        <p>수집 기간: {df['Timestamp'].min().strftime('%Y-%m-%d %H:%M:%S')} ~ {df['Timestamp'].max().strftime('%Y-%m-%d %H:%M:%S')} (총 {duration_str})</p>
    </div>

    <div class="summary-cards">
        <div class="card">
            <div class="card-title">평균 온도</div>
            <div class="card-value">{avg_temp:.1f} °C</div>
        </div>
        <div class="card">
            <div class="card-title">평균 습도</div>
            <div class="card-value">{avg_hum:.1f} %</div>
        </div>
        <div class="card sound">
            <div class="card-title">평균 소음</div>
            <div class="card-value">{avg_db:.1f} dB</div>
        </div>
        <div class="card sound">
            <div class="card-title">최대 소음</div>
            <div class="card-value">{max_db:.1f} dB</div>
        </div>
        <div class="card motion">
            <div class="card-title">움직임 발생률</div>
            <div class="card-value">{motion_ratio:.1f} %</div>
        </div>
    </div>

    <div class="chart-container">
        <h3>센서 트렌드 및 활동 분석 차트</h3>
        <img src="{chart_path}" alt="Sensor Trends Chart">
    </div>

    <div class="search-bar">
        <input type="text" id="searchInput" onkeyup="filterTable()" placeholder="검색어 입력 (카테고리, 날짜, 상태 등)...">
    </div>

    <div class="table-container">
        <table id="logTable">
            <thead>
                <tr>
                    <th>Timestamp</th>
                    <th>Temperature</th>
                    <th>Humidity</th>
                    <th>Motion</th>
                    <th>Sound RMS</th>
                    <th>Sound dB</th>
                    <th>Acoustic Category</th>
                    <th>BLE Count</th>
                    <th>Occupancy Status</th>
                </tr>
            </thead>
            <tbody>
                {rows_joined}
            </tbody>
        </table>
    </div>
</body>
</html>
"""

with open(html_path, "w", encoding="utf-8") as f:
    f.write(html_content)

print(f"[완료] 3. 대화형 조건부 서식 HTML 생성 완료: {html_path}")
print("==================================================")
print("[알림] 모든 데이터 가공이 완료되었습니다.")
