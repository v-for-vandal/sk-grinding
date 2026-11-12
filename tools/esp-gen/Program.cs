using System.Text.RegularExpressions;
using System.Security.Cryptography;
using System.Text.Json;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

if(args.Length>0 && args[0]=="--sources") { Sources.Extract(args.Skip(1).ToArray()); return; }
if(args.Length>0 && args[0]=="--schema") {
 foreach(var type in typeof(Quest).Assembly.GetTypes().Where(t=>args.Skip(1).Contains(t.Name))) {
 Console.WriteLine(type.FullName);foreach(var p in type.GetProperties())Console.WriteLine($"  {p.Name}: {p.PropertyType}");
 if(type.IsEnum)Console.WriteLine(string.Join(", ",Enum.GetNames(type)));
 }return;
}
var root = Path.GetFullPath(args.Length > 0 ? args[0] : ".");
var output = Path.Combine(root, "dist", "SkillPractice");
Directory.CreateDirectory(output);
var names = Regex.Matches(File.ReadAllText(Path.Combine(root,"config","kid","SPO_WeaponClass_KID.ini")), @"SPO_WeapClass_[A-Za-z0-9_]+")
    .Select(x=>x.Value).Distinct().Order(StringComparer.Ordinal).ToArray();
if(names.Length != 22) throw new Exception($"Expected 22 keywords, got {names.Length}");
var mod = new SkyrimMod(ModKey.FromNameAndExtension("SkillPractice.esp"), SkyrimRelease.SkyrimSE);
mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;
mod.ModHeader.MasterReferences.Add(new MasterReference { Master = ModKey.FromNameAndExtension("Skyrim.esm") });
uint id=0x800;
var rows=new List<object>();
foreach(var name in names) {
    var kw = new Keyword(new FormKey(mod.ModKey,id++),SkyrimRelease.SkyrimSE) { EditorID = name };
    mod.Keywords.Add(kw); rows.Add(new {editorID=name,localID=$"0x{kw.FormKey.ID:X6}"});
}
var quest = new Quest(new FormKey(mod.ModKey,0x816),SkyrimRelease.SkyrimSE) {
    EditorID="SkillPractice_MCM", Name="Skill Practice", Flags=Quest.Flag.StartGameEnabled,
    Priority=0, NextAliasID=1,
    VirtualMachineAdapter=new QuestAdapter { Version=5,ObjectFormat=2 }
};
var mcmScript=new ScriptEntry { Name="SkillPractice_MCMScript" };
mcmScript.Properties.Add(new ScriptStringProperty {Name="ModName",Data="SkillPractice",Flags=ScriptProperty.Flag.Edited});
quest.VirtualMachineAdapter.Scripts.Add(mcmScript);
var player = new FormKey(ModKey.FromNameAndExtension("Skyrim.esm"),0x14);
var alias=new QuestAlias {ID=0,Name="PlayerAlias"};
alias.ForcedReference.SetTo(player);quest.Aliases.Add(alias);
var fragment=new QuestFragmentAlias {Version=5,ObjectFormat=2,Property=new ScriptObjectProperty {Alias=0,Flags=ScriptProperty.Flag.Edited}};
fragment.Property.Object.SetTo(quest.FormKey);
fragment.Scripts.Add(new ScriptEntry {Name="SKI_PlayerLoadGameAlias"});
quest.VirtualMachineAdapter.Aliases.Add(fragment);
mod.Quests.Add(quest);
var path=Path.Combine(output,"SkillPractice.esp");
mod.WriteToBinary(path);
using(var check=SkyrimMod.CreateFromBinaryOverlay(path,SkyrimRelease.SkyrimSE)) {
 if(check.Keywords.Count!=22||check.Quests.Count!=1||check.Quests.First().Aliases.Count!=1||check.Quests.First().VirtualMachineAdapter?.Aliases.Count!=1)throw new Exception("Generated ESP failed read-back validation");
}
File.WriteAllText(Path.Combine(root,"tools","esp-gen","keywords.json"),JsonSerializer.Serialize(rows,new JsonSerializerOptions{WriteIndented=true})+"\n");
Console.WriteLine($"Wrote {path}: 22 keywords; SHA256 {Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)))}");
