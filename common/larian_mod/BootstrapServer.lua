-- Party members carry the mod's status all the time, allies outside the party while they are in combat
local status = "@STATUS@"

-- Reapplied, so a status from a save picks up the boosts as they are now
local function apply (character)
	Osi.RemoveStatus (character, status)
	Osi.ApplyStatus (character, status, -1)
end

local function isPartyMember (character)
	return #Osi.DB_PartyMembers:Get (character) > 0
end

local function isAllyOutsideParty (object)
	return Osi.IsCharacter (object) == 1 and not isPartyMember (object) and Osi.IsAlly (object, Osi.GetHostCharacter ()) == 1
end

Ext.Osiris.RegisterListener ("LevelGameplayStarted", 2, "after", function ()
	for _, row in pairs (Osi.DB_PartyMembers:Get (nil)) do
		apply (row[1])
	end
end)

Ext.Osiris.RegisterListener ("CharacterJoinedParty", 1, "after", apply)

Ext.Osiris.RegisterListener ("CharacterLeftParty", 1, "after", function (character)
	Osi.RemoveStatus (character, status)
end)

Ext.Osiris.RegisterListener ("EnteredCombat", 2, "after", function (object, _)
	if isAllyOutsideParty (object) then
		apply (object)
	end
end)

Ext.Osiris.RegisterListener ("LeftCombat", 2, "after", function (object, _)
	if not isPartyMember (object) then
		Osi.RemoveStatus (object, status)
	end
end)
