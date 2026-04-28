# Python调用示例
import requests

url = "https://api-v2.cenguigui.cn/api/speech/AiChat/?module=audio&text=人人都看不起我，偏偏我也不挣钱！&voice=曼波"

response = requests.get(url)

print(response.json())