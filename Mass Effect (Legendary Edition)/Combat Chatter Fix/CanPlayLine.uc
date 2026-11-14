public function bool CanPlayLine(BioPawn.EBioSoundSetType SoundSet)
{
    local BioWorldInfo TheWorldInfo;
    local int Index;
    
    TheWorldInfo = BioWorldInfo(Class'Engine'.static.GetCurrentWorldInfo());
    if (TheWorldInfo == None)
    {
        return FALSE;
    }
    for (Index = 0; Index < m_LastPlayedTimes.Length; Index++)
    {
        if (int(m_LastPlayedTimes[Index].SoundSet) == int(SoundSet))
        {
            if (TheWorldInfo.GameTimeSeconds < m_LastPlayedTimes[Index].TimeLastPlayed || TheWorldInfo.GameTimeSeconds - m_LastPlayedTimes[Index].TimeLastPlayed >= m_MinTime[int(SoundSet)])
            {
                return TRUE;
            }
            return FALSE;
        }
    }
    return TRUE;
}