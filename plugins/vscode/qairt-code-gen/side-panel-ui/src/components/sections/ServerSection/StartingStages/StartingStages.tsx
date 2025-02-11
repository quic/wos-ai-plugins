import { ServerStartingStage } from '@shared/server-state';
import { ReactElement } from 'react';
import { VscodeIcon } from '../../../shared/VscodeIcon/VscodeIcon';
import './StartingStages.css';

const startingStages = [
  ServerStartingStage.START_SERVER,
];

const startingStagesLabelsMap = {
  [ServerStartingStage.START_SERVER]: 'Starting Model',
};

interface StartingStageProps {
  stage: ServerStartingStage;
  icon: ReactElement<typeof VscodeIcon>;
}

const StartingStage = ({ stage, icon }: StartingStageProps): JSX.Element => {
  return (
    <span className="starting-stage-item">
      {icon}&nbsp;{startingStagesLabelsMap[stage]}
    </span>
  );
};

const getStageIcon = (
  itemStage: ServerStartingStage,
  currentStage: StartingStagesProps['currentStage']
): ReactElement<typeof VscodeIcon> => {
  if (currentStage === itemStage) {
    return <VscodeIcon iconName="loading" spin></VscodeIcon>;
  } else if (currentStage && currentStage > itemStage) {
    return <VscodeIcon iconName="check"></VscodeIcon>;
  } else {
    return <VscodeIcon iconName="debug-pause"></VscodeIcon>;
  }
};

interface StartingStagesProps {
  currentStage: ServerStartingStage | null;
}

export const StartingStages = ({ currentStage }: StartingStagesProps): JSX.Element => {
  return (
    <pre className="starting-stages">
      {startingStages.map((itemStage) => (
        <StartingStage key={itemStage} stage={itemStage} icon={getStageIcon(itemStage, currentStage)}></StartingStage>
      ))}
    </pre>
  );
};
