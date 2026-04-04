## ADDED Requirements

### Requirement: Connection state machine manages connection lifecycle
The system SHALL manage the connection lifecycle through explicit states: Disconnected, Connecting, Connected, Reconnecting, and ReconnectFailed. The state machine MUST transition appropriately based on connection attempts, successes, and failures.

#### Scenario: Initial state
- **WHEN** application starts
- **THEN** connection state is Disconnected

#### Scenario: User initiates connection
- **WHEN** user clicks "Connect" button or calls Start() from Disconnected state
- **THEN** state transitions to Connecting
- **AND** HTTP /start API is invoked

#### Scenario: Connection succeeds
- **WHEN** /start API returns success AND WebSocket connection is established
- **THEN** state transitions to Connected
- **AND** OnBliveConnected event is invoked

#### Scenario: Connection fails
- **WHEN** /start API returns error
- **THEN** state remains Disconnected
- **AND** error is logged

### Requirement: Disconnect reason tracking
The system SHALL track and expose the reason for disconnection, including: None (active disconnect), NetworkError, HeartbeatTimeout, ServerClose, and AuthFailed.

#### Scenario: Active disconnect
- **WHEN** user clicks "Disconnect" button or calls Disconnect()
- **THEN** disconnect reason is set to None

#### Scenario: Network error disconnect
- **WHEN** network request fails (HTTP or WebSocket)
- **THEN** disconnect reason is set to NetworkError

#### Scenario: Heartbeat timeout disconnect
- **WHEN** heartbeat response is not received within timeout
- **THEN** disconnect reason is set to HeartbeatTimeout

#### Scenario: Server closes connection
- **WHEN** server sends close frame or INTERACTION_END message
- **THEN** disconnect reason is set to ServerClose

### Requirement: Automatic reconnection with retry limit
The system SHALL automatically attempt reconnection when disconnected due to network error, heartbeat timeout, or server close. The system MUST limit reconnection attempts to 5 times maximum.

#### Scenario: Automatic reconnection triggered
- **WHEN** disconnection occurs with reason != None AND retry count < 5
- **THEN** state transitions to Reconnecting
- **AND** retry count is incremented
- **AND** Start() is called immediately

#### Scenario: Reconnection succeeds
- **WHEN** reconnection attempt succeeds (WebSocket connected)
- **THEN** state transitions to Connected
- **AND** retry count is reset to 0

#### Scenario: Reconnection fails
- **WHEN** reconnection attempt fails AND retry count >= 5
- **THEN** state transitions to ReconnectFailed
- **AND** automatic reconnection stops

#### Scenario: In-flight heartbeat ignored after disconnect
- **WHEN** user actively disconnects (reason = None)
- **AND** an in-flight heartbeat response returns with error
- **THEN** the heartbeat response is ignored
- **AND** automatic reconnection is NOT triggered

### Requirement: Manual reconnection after failure
The system SHALL allow manual reconnection after reaching maximum retry limit.

#### Scenario: User initiates manual reconnection
- **WHEN** user clicks "Connect" button while in ReconnectFailed state
- **THEN** retry count is reset to 0
- **AND** state transitions to Connecting
- **AND** Start() is called

### Requirement: Active disconnect clears retry count
The system SHALL clear retry count when user actively disconnects.

#### Scenario: User actively disconnects
- **WHEN** user clicks "Disconnect" button or calls Disconnect()
- **THEN** retry count is reset to 0
- **AND** state transitions to Disconnected
- **AND** disconnect reason is set to None

### Requirement: UI displays connection state and reason
The system SHALL display the current connection state, reason (when applicable), and retry progress to the user.

#### Scenario: Display connected state
- **WHEN** state is Connected
- **THEN** UI shows "已连接"

#### Scenario: Display connecting state
- **WHEN** state is Connecting
- **THEN** UI shows "连接中..."

#### Scenario: Display reconnecting progress
- **WHEN** state is Reconnecting
- **THEN** UI shows "正在重连 (N/5)..."

#### Scenario: Display reconnect failed with reason
- **WHEN** state is ReconnectFailed
- **THEN** UI shows "重连失败，原因: <reason>"

#### Scenario: Display disconnected state
- **WHEN** state is Disconnected AND reason is None
- **THEN** UI shows "未连接"

### Requirement: UI button text reflects current state
The system SHALL display appropriate button text based on current connection state.

#### Scenario: Show Connect button
- **WHEN** state is Disconnected OR ReconnectFailed
- **THEN** button text is "连接"

#### Scenario: Show Disconnect button
- **WHEN** state is Connected
- **THEN** button text is "断开"

#### Scenario: Show Cancel button
- **WHEN** state is Connecting OR Reconnecting
- **THEN** button text is "取消"

### Requirement: Cancel operation during connection attempt
The system SHALL allow canceling connection or reconnection attempts.

#### Scenario: User cancels connection
- **WHEN** user clicks "Cancel" button during Connecting OR Reconnecting
- **AND** Disconnect() is called
- **THEN** state transitions to Disconnected
- **AND** disconnect reason is set to None
- **AND** retry count is reset to 0
- **AND** pending network operations are cleaned up
