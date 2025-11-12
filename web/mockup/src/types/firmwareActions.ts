export interface FirmwareActionParam {
  name: string;
  type: "string" | "number" | "boolean";
  description?: string;
}

export interface FirmwareActionDefinition {
  id: string;
  label: string;
  description?: string;
  category?: string;
  params?: FirmwareActionParam[];
}

export interface FirmwareActionManifest {
  updatedAt: string;
  actions: FirmwareActionDefinition[];
}
