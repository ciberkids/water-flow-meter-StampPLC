export interface SimulationTraceEntry {
  id: string;
  label?: string;
  trigger: string;
  screenId: string;
  screenName?: string;
  functionName?: string;
  timestamp: number;
  actionParams?: Record<string, unknown> | null;
  targetScreenId?: string;
  notes?: string;
}
