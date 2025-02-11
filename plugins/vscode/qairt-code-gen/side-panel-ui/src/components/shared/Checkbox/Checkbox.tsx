import { ReactNode } from 'react';
import './Checkbox.css';

interface CheckboxProps {
  checked?: boolean;
  children: ReactNode;
  onChange: (isChecked: boolean) => void;
  disabled: boolean;
}

export const Checkbox = ({ checked, children, onChange, disabled }: CheckboxProps): JSX.Element => {
  const classNames = ['vscode-checkbox', 'codicon'];
  if (checked) {
    classNames.push('codicon-check');
  }
  return (
    <div className="checkbox" aria-disabled={disabled}>
      <div aria-disabled={disabled} className={classNames.join(' ')} onClick={() => onChange(!checked)}></div>
      <span aria-disabled={disabled} className="checkbox-label">{children}</span>
    </div>
  );
};
