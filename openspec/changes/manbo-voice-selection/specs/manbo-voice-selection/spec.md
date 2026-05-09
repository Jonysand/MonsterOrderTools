## ADDED Requirements

### Requirement: C# UI can fetch and display voice list
The system SHALL provide a mechanism for the C# UI layer to asynchronously fetch the list of available Manbo TTS voices from the external API at program startup.

#### Scenario: Successful API fetch
- **WHEN** the application starts and the network is available
- **THEN** the C# layer SHALL send a GET request to `https://api.milorapart.top/apis/AIvoice/?type=list`
- **AND** parse the JSON response to extract the `speakers` array
- **AND** cache the voice list in memory for UI use

#### Scenario: Failed API fetch
- **WHEN** the application starts and the API request fails (network error, HTTP error, or parse error)
- **THEN** the system SHALL log the error
- **AND** the voice list SHALL contain only the default voice "曼波（付费）"

#### Scenario: API returns empty speakers list
- **WHEN** the API responds with an empty `speakers` array
- **THEN** the voice list SHALL contain only "曼波（付费）"

### Requirement: UI provides voice selection control
The system SHALL present a ComboBox in the Manbo configuration panel that allows the user to select a voice.

#### Scenario: Display voice list in ComboBox
- **WHEN** the user opens the voice settings tab
- **THEN** the ComboBox SHALL display "曼波（付费）" as the first item
- **AND** all other voices from the API SHALL be listed below it
- **AND** each item SHALL have the voice name as display text and the same voice name as its Tag value

#### Scenario: Persist voice selection
- **WHEN** the user selects a voice from the ComboBox
- **THEN** the system SHALL update the `MANBO_VOICE` configuration field
- **AND** trigger a `ConfigChanged` event with the key `MANBO_VOICE`

#### Scenario: Restore saved voice on load
- **WHEN** the configuration window initializes
- **THEN** the ComboBox SHALL select the voice matching the saved `MANBO_VOICE` value
- **AND** if the saved voice is not in the fetched list, the ComboBox SHALL still select that voice (preserve user choice)

### Requirement: C++ layer routes TTS requests based on voice
The C++ `ManboTTSProvider` SHALL route TTS requests to the correct API endpoint based on the configured voice.

#### Scenario: Request with "曼波" voice
- **WHEN** a TTS request is made and `config.manboVoice` equals "曼波"
- **THEN** the provider SHALL use the existing endpoint `/apis/mbAIscvip`
- **AND** include parameters `text`, `format=mp3`, `speed`, and `key`

#### Scenario: Request with other voice
- **WHEN** a TTS request is made and `config.manboVoice` is not "曼波"
- **THEN** the provider SHALL use the endpoint `/apis/AIvoice`
- **AND** include parameters `speaker=<voice>` and `text=<text>`
- **AND** use the same `Authorization: Bearer <manboApiKey>` header

#### Scenario: Parse response from new endpoint
- **WHEN** the `/apis/AIvoice` endpoint returns a JSON response
- **THEN** the provider SHALL parse it using the same logic as the existing endpoint
- **AND** extract the `url` field when `code` is 200

### Requirement: Configuration field syncs across layers
The `manboVoice` configuration field SHALL synchronize bidirectionally between C++ and C# layers.

#### Scenario: Load configuration
- **WHEN** the application loads configuration from JSON or registry
- **THEN** the C++ `ConfigData.manboVoice` SHALL be initialized with the saved value or default to "曼波"
- **AND** the value SHALL be exposed to C# via `DataBridgeWrapper.ConfigProxy.ManboVoice`

#### Scenario: Save configuration
- **WHEN** the user saves settings from the UI
- **THEN** the C# `MainConfig.MANBO_VOICE` value SHALL be written back to C++ `ConfigData`
- **AND** persisted to the JSON configuration file

### Requirement: Backward compatibility
The system SHALL maintain backward compatibility with existing configurations that do not contain the `MANBO_VOICE` field.

#### Scenario: Load old configuration without voice field
- **WHEN** an old configuration file is loaded that lacks the `MANBO_VOICE` field
- **THEN** the system SHALL default the voice to "曼波"
- **AND** TTS requests SHALL continue to use the existing `/apis/mbAIscvip` endpoint
