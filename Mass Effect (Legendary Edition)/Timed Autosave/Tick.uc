public function Tick(float fDeltaT)
{
    local BioPlayerController PC;
    local SFXEngine Engine;
    
    if (m_fNoBrowserWheelTimer > 0.0)
    {
        m_fNoBrowserWheelTimer -= fDeltaT;
    }
    PC = GetLocalPlayerController();
    if (!bPlayersOnly && !HasFocus() && PC != None && PC.GameModeManager != None && PC.GameModeManager.CanPause())
    {
        PC.GameModeManager.TurnOffStormForPause();
        m_bPausedByFocusLoss = TRUE;
        bPlayersOnly = TRUE;
    }
    else if (HasFocus() && m_bPausedByFocusLoss)
    {
        m_bPausedByFocusLoss = FALSE;
        bPlayersOnly = FALSE;
    }
    TimerList.Tick(fDeltaT);
    UpdateSubtitles(fDeltaT);
    UpdateConversation(fDeltaT);
    UpdateSlowMotionQ(fDeltaT);
    if (m_bSetGameSpeed)
    {
        Game.SetGameSpeed(m_fCurrentSlomo);
        m_bSetGameSpeed = FALSE;
    }
    if (WindEnableFake == 1)
    {
        DoFakeWind(fDeltaT);
    }
    if (m_oPropertyEditor != None)
    {
        m_oPropertyEditor.UpdateSystem(fDeltaT);
    }
    UpdateVFXStats(fDeltaT);
    UpdateVFXPools(fDeltaT);
    if (CurrentGame != None && !CurrentGame.m_bSavingGame)
    {
        Engine = Class'SFXEngine'.static.GetEngine();
        if (Engine.LastSaveTime > CurrentGame.m_fLastSaveTime)
        {
            CurrentGame.m_fLastSaveTime = Engine.LastSaveTime;
        }
        CurrentGame.m_bDissableAutoSave = FALSE;
        CurrentGame.m_fAutoSaveAttemptDelay = CurrentGame.m_fAutoSaveTime;
        CurrentGame.TryAutoSaving();
    }
}