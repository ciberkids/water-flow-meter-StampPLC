/**
 * The app's view of the firmware manifest.
 *
 * These are aliases of the canonical types in shared/schemaDefinitions.ts rather
 * than independent declarations. They used to be a parallel definition that
 * required `name` where the canonical schema required `label`; keeping them as
 * aliases means the UI cannot drift from what the exporter validates.
 */
import type {
  FirmwareAction,
  FirmwareManifest,
  FirmwareValue
} from "../../shared/schemaDefinitions";

export type FirmwareActionDefinition = FirmwareAction;
export type FirmwareValueDefinition = FirmwareValue;
export type FirmwareActionManifest = FirmwareManifest;
