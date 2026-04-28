Add-Type -Path "D:\VisualStudioProjects\JonysandMHDanmuTools\packages\SQLite.3.13.0\lib\net45\SQLite.dll"
$conn = New-Object System.Data.SQLite.SQLiteConnection("Data Source=D:\VisualStudioProjects\JonysandMHDanmuTools\x64\Debug\MonsterOrderWilds_configs\captain_profiles.db")
$conn.Open()
$cmd = $conn.CreateCommand()
$cmd.CommandText = "SELECT uid, username, last_checkin_date, continuous_days FROM user_profiles LIMIT 10"
$reader = $cmd.ExecuteReader()
while ($reader.Read()) {
    Write-Host ("uid=" + $reader.GetValue(0) + " username=" + $reader.GetString(1) + " last_checkin_date=" + $reader.GetValue(2) + " continuous_days=" + $reader.GetValue(3))
}
$reader.Close()
$conn.Close()