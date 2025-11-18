# app.py
from flask import Flask, jsonify, render_template
import mysql.connector

# --- Configuration ---
DB_CONFIG = {
    'user': 'sensor_user',
    'password': 'password', # <-- 비밀번호를 확인하세요
    'host': '127.0.0.1',
    'database': 'sensor_db'
}
NUM_POINTS_TO_FETCH = 50
SMA_WINDOW_SIZE = 5 # 이동 평균을 계산할 데이터 개수 (5개 평균)

app = Flask(__name__)

def get_db_connection():
    conn = mysql.connector.connect(**DB_CONFIG)
    return conn

# --- AI 분석 함수: 단순 이동 평균 계산 ---
def calculate_sma(data, window_size):
    """Calculates the Simple Moving Average for a list of data."""
    if not data or len(data) < window_size:
        return [] # 데이터가 윈도우 크기보다 작으면 계산 불가
    
    sma = []
    for i in range(len(data) - window_size + 1):
        window = data[i : i + window_size]
        window_average = sum(window) / window_size
        sma.append(window_average)
    return sma

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/data')
def get_data():
    conn = get_db_connection()
    cursor = conn.cursor()
    
    # 최종적으로 반환될 JSON 구조
    # 예: {"random_simulator": {"raw": [...], "sma": [...]}, ...}
    response_data = {}
    sensors_to_query = ['random_simulator', 'esp32/cds']
    
    for sensor_name in sensors_to_query:
        query = f"""
            SELECT r.value 
            FROM readings r JOIN sensors s ON r.sensor_id = s.sensor_id
            WHERE s.sensor_name = '{sensor_name}'
            ORDER BY r.timestamp DESC LIMIT {NUM_POINTS_TO_FETCH}
        """
        cursor.execute(query)
        results = cursor.fetchall()
        
        # 원본 데이터 (시간 순서대로)
        raw_data = [float(item[0]) for item in reversed(results)]
        
        # 이동 평균 데이터 계산
        sma_data = calculate_sma(raw_data, SMA_WINDOW_SIZE)
        
        # 최종 결과에 추가
        response_data[sensor_name] = {
            "raw": raw_data,
            "sma": sma_data
        }

    cursor.close()
    conn.close()
    return jsonify(response_data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)

