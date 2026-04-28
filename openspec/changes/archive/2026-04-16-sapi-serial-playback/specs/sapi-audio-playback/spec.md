# SAPI Audio Playback

## ADDED Requirements

### Requirement: SAPI TTS serial playback without temp files

The system SHALL route all SAPI TTS playback through an event-driven serial mechanism without creating temporary files.

#### Scenario: SAPI TTS queued for playback
- **WHEN** a TTS request with SAPI engine is processed
- **THEN** the request SHALL be added to the asyncPendingQueue_ with state Playing
- **AND** the system SHALL use SPF_ASYNC flag for non-blocking playback
- **AND** the system SHALL set up SPEVENT callback to detect playback completion

#### Scenario: SAPI waits for playback completion before next request
- **WHEN** a SAPI TTS request is at the front of the queue with state Playing
- **THEN** the system SHALL wait for SPEI_STREAM_ENDED event from SAPI
- **AND** no other TTS request SHALL be processed while SAPI is playing
- **AND** after SPEI_STREAM_ENDED, mark request as Completed and process next

#### Scenario: Bubble callback before playback
- **WHEN** a checkin TTS request with SAPI engine is processed
- **THEN** g_checkinTTSPlayCallback SHALL be invoked before the playback starts
- **AND** the UI SHALL display the bubble immediately

### Requirement: SAPI TTS failure handling

The system SHALL handle SAPI playback failures gracefully without blocking the queue.

#### Scenario: SAPI playback fails immediately
- **WHEN** ISpVoice::Speak returns a failure HRESULT
- **THEN** the request SHALL be marked as Failed with error message "SAPI speak failed"
- **AND** the system SHALL proceed to process the next request

#### Scenario: SAPI playback timeout
- **WHEN** SAPI playback does not complete within a reasonable timeout
- **THEN** the request SHALL be marked as Failed with error message "SAPI playback timeout"
- **AND** the system SHALL proceed to process the next request

### Requirement: SAPI playback completion callback

The system SHALL use Windows SAPI event mechanism to detect playback completion.

#### Scenario: SPEVENT callback triggered
- **WHEN** ISpVoice generates SPEI_STREAM_ENDED event
- **THEN** the callback SHALL mark the current SAPI request as Completed
- **AND** the callback SHALL trigger ProcessAsyncTTS to process the next request